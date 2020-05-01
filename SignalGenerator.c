/*
 * R2R_ADC_wEpwm.c
 *
 *  Created on: 30/04/2020
 *      Author: Jkiol115
 */
// Por medio de un R2R se genera una señal ya sea esta sierra, cuadrada, cuadratica, seno
// se cambian los parametros Fout(Hz), A(bits), WF(wave form)

#include "F28x_Project.h"
#include "math.h"
    // ClkCfgRegs.PERCLKDIVSEL
    //          Fsysclkout(50Meg)
    //TBPRD = ----------------------------
    //         2 * (Fpwm(1k)) * CLKDIN(1) *HSPCLKDIN(1)
    // Configurar TBPRD
    //**********************************************************************************************
    //********************************definiciones del programa*************************************
    //**********************************************************************************************
#define EPWM1_TIMER_TBPRD   98
#define BUFFER              256
#define F_in                500
#define acqps               14
#define FREE_RUN_CTR        2
    //**********************************************************************************************
    //**********************************Variables del programa**************************************
    //**********************************************************************************************
typedef struct {
    Uint16 A_rgs;
    Uint32 Fout_rgs;
    Uint16 WF_rgs;
}rgs;
    //**********************************************************************************************
    //**********************************prototipos del programa*************************************
    //**********************************************************************************************
void InitPWM_R2R(void);
void InitR2R(void);
void InitADCA(void);
void InitPWM_ADCA(void);
void WaveForm(int set);
interrupt void Generator(void);
interrupt void Sample_ADC(void);
    //**********************************************************************************************
    //****************************************Variables********************************************
    //**********************************************************************************************
Uint16 Fn[BUFFER];
Uint16 n = 0;

volatile Uint16 A = ((BUFFER)/2) - 1;
volatile float32 w = 2*3.1416/BUFFER;
volatile Uint16 WF = 0;
volatile Uint32 Fout = F_in;

volatile float32 xn[BUFFER];
Uint16 n1 = 0;
Uint16 adq_done = 0;
void main(void)
{
    rgs usr = { A, Fout, 0 };
    //**********************************************************************************************
    //********************************incialzacion del sistema**************************************
    //**********************************************************************************************
    // configurar la tarjeta con condiciones iniciales, el archivo de InitSysCtrl
    InitSysCtrl();
    DINT;                                   //deshabilitar interrupciones
    InitPieCtrl();                          //inicializar vector interrupciones
    IER = 0x0000;                           //Interrupt Enable Register
    IFR = 0x0000;                           //Interrupt Flag Registe
    InitPieVectTable();                     //Inicializar Perif. Interr.

    EALLOW;
    PieVectTable.EPWM1_INT = &Generator;    //dar direccion de tratamiento de int.
    PieVectTable.ADCA1_INT = &Sample_ADC;
    EDIS;

    InitADCA();
    InitPWM_ADCA();
    InitPWM_R2R();                              //inicializar PWM. demás conf adentro de la funcion
    InitR2R();                              //Inicializar GPIO que se utilzian.

    IER |= M_INT1;                          //hab. grupo 1
    IER |= M_INT3;                          //Habilitar grupo int3
    EINT;                                   //hab. interrupciones
    ERTM;                                   //DBGM int.

    WaveForm(WF); // defaul seno

    for(n1 = 0; n1 < BUFFER; n1++){
        xn[n1] = 0;
    }
    n1 = 0;
    adq_done = 0;

    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;      //habilitar interrupcion
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;      //habilitar interrupcion

    EALLOW;
    EPwm1Regs.TBCTR = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;
    EPwm2Regs.ETSEL.bit.SOCAEN = 1;             //Habilitar SOCA
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;  //contar arriba y abajo
    EDIS;
    //**********************************************************************************************
    //*********************************logica general de programa***********************************
    //**********************************************************************************************

    for(;;)
    {
        if (Fout != usr.Fout_rgs){
            EALLOW;
            EPwm1Regs.TBPRD = (int)((Uint32)25000000/((Uint32)Fout*BUFFER));            //perdiodo de del time-based
            EDIS;
            usr.Fout_rgs = Fout;
        }
        if(WF != usr.WF_rgs){
            if(usr.WF_rgs == 0)
                A *= 2;
            WaveForm(WF);
            usr.WF_rgs = WF;
        }
        if(A != usr.A_rgs){
            WaveForm(WF);
            usr.A_rgs = A;
        }

        if (adq_done == 1){
            adq_done = 0;
        }
        asm("   NOP");
    }

}


//**********************************************************************************************
//******************************Tratamiento interrupciones**************************************
//**********************************************************************************************
interrupt void Generator(void)
{
    GpioDataRegs.GPADAT.all = Fn[n++];
    if (n >= BUFFER) n = 0;
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
interrupt void Sample_ADC(void)
{
    xn[n1++] = (float32)AdcaResultRegs.ADCRESULT0*3/4096;

    if(BUFFER <= n1)
    {
        n1 = 0;
        adq_done = 1;
    }

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;      //limpiar bandera
    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;  //clear INT1 overflow flag
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  //clear INT1 flag
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
//**********************************************************************************************
//******************************funciones de programa*******************************************
//**********************************************************************************************


void InitADCA(void)
{
    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE       = 6;                                  //dividir por /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);  //configurar adc
    //configurado para el ADC_a, con resolucion de 12 bist, entrada unica(tmbn puede ser diferencial)
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS    = 1;                               //ubicacion de la interrupcion
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ       = 1;                                  //encender ADC
    DELAY_US(1000);                                                     //dar tiempo para inicializar
    AdcaRegs.ADCSOC0CTL.bit.CHSEL       = ADC_CHANNEL_0;                      //utilizar A0 y SOC0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS       = acqps;                              //sample window is 100 SYSCLK cycles
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL     = 7;                                //disparo en ePWM1 SOCA/C
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL   = 0;                              //EOC dspara interrupcion
    AdcaRegs.ADCINTSEL1N2.bit.INT1E     = 1;                                //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1   = 1;                              //make sure INT1 flag is cleared
    EDIS;
}
void InitPWM_ADCA(void){
    EALLOW;
    CpuSysRegs.PCLKCR2.bit.EPWM2  = 1;               //encender modulo PWM
    EPwm2Regs.ETSEL.bit.SOCAEN    = 0;              // desh. SOC en grupo A
    EPwm2Regs.ETSEL.bit.SOCASEL   = ET_CTRU_CMPA;   // SOC en flanco de subida
    EPwm2Regs.ETPS.bit.SOCAPRD    = ET_1ST;         // pulso en primer evento
    EPwm2Regs.CMPA.bit.CMPA       = 12500;          // comparar a mitad de TBPRD
    EPwm2Regs.TBPRD               = 25000;          // 1kHz
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        //clk div hi presition
    EPwm1Regs.TBCTL.bit.CLKDIV    = TB_DIV1;        //clk div
    EPwm2Regs.TBCTL.bit.CTRMODE   = TB_FREEZE;      // freeze counter
    EDIS;
}


// 1 k [Hz] * 256
void InitPWM_R2R(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;               //encender modulo PWM

    EPwm1Regs.TBPRD = (int)((Uint32)25000000/((Uint32)Fout*BUFFER));            //perdiodo de del time-based
    EPwm1Regs.TBPHS.bit.TBPHS = 0x0000;             //forzar fase
    EPwm1Regs.TBCTR = 0x0000;                       //time based counter forzado en cero
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;  //modo del contador es hacia UPDOWN
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;         //deshabilitar carga de fase
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        //clk div hi presition
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;           //clk div
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;            //comparador para apagado, en cero
    EPwm1Regs.AQCTLA.bit.PRD = AQ_SET;              //comparador para encendido, en TBPRD(max)
    EPwm1Regs.TBCTL.bit.FREE_SOFT = FREE_RUN_CTR;

    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;       //interrupcion al llegar a cero
    EPwm1Regs.ETSEL.bit.INTEN = TB_ENABLE;          //habilitar interrupcion
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;             //en el primer evento
    EDIS;
    if(Fout == 1){
        EPwm1Regs.TBPRD = 28828;
        EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV2;           //clk div
    }
}
void InitR2R(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.all     = 0x00FF; // puertos 7-0 de salida
    GpioCtrlRegs.GPAMUX1.all    = 0x0000; // periferico GPIO
    GpioCtrlRegs.GPAPUD.all     = 0x00FF; // pull-up disable
    EDIS;
    GpioDataRegs.GPACLEAR.all   = 0xFFFF; // limpiar todo
}
void WaveForm(int set)
{
    int i;
    if(A >= BUFFER)
        A = BUFFER;
    switch (set)
    {
        case 0: //seno
            if(A >= ((BUFFER/2)))
                A = A/2;
            for(i = 0; i < BUFFER; i++)
                Fn[i] = A*(sin(w*i) + 1);
            break;

        case 1: // rampa
            for(i = 0; i < BUFFER; i++)
                Fn[i] = (A*i)/BUFFER;
            break;

        case 2: // cuadrada
            for(i = 0; i < (BUFFER/2); i++){
                Fn[i] = A;
                Fn[i + (BUFFER/2)] = 0;
            }
            break;
        case 3: // cuadratica
            for(i = 0; i < BUFFER; i++){
                Fn[i] = (int)(A*(((float)i)/(BUFFER)));
                Fn[i] = (int)(Fn[i]*(((float)i)/(BUFFER)));
            }
            break;
        default:
            break;
    }
}



