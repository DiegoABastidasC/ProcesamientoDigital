/*
 * Genetaror_PWM_79d.c
 *
 *  Created on: 28/04/2020
 *      Author: Jkiol115
 */
/**
 * main.c
 */

// Realizar una DFT para N puntos(soportado por memoria). si se quiere cambiar la forma de onda se debe ajustar xn
// xn es inicializada en codigo como una funcion rampa, igual para la cantidad de datos a la cual es aplicada la DFT
// es definida por N_Size, ed decir, si se quiere relizar el proceso para N mas grandes se debe cambiar BUFFER y N_Size
// para que sean iguales, no optimizado debido a tiempo de desarrollo.
// para visualizar registros debe ir View->Expressions->digitar nombre de variable que se desea ver

#include "F28x_Project.h"
#include "math.h"

    //**********************************************************************************************
    //********************************definiciones del programa*************************************
    //**********************************************************************************************
#define PI 3.14159
#define BUFFER 4
#define N_Size 4
    //**********************************************************************************************
    //**********************************prototipos del programa*************************************
    //**********************************************************************************************

    //**********************************************************************************************
    //****************************************Variables********************************************
    //**********************************************************************************************
    float32 xn[BUFFER];             //aquí se guarda la funcion muestreada para este caso se genera por software
    float32 xk[BUFFER][BUFFER];     //DFT
    float32 mag[BUFFER];              //magnitud
    int16 k, n;                     //apuntadores para loops
    float32 w[BUFFER][BUFFER][2];   //calculo para matriz w_nk
    int16 N = N_Size;               //la cantidad de datos que se utilizan
void main(void)
{
    //se inicializa el dispositivo en un estado conocido
    InitSysCtrl();
    //Iniciar con rampa xn
    for(n = 0; n < BUFFER; n++){
        // la funcion de inicializa como una rampa
        xn[n] = n;
    }
    //iniciar en cero Xk
    for (n = 0; n < N; n++)
    {
        for (k = 0; k < N; k++)
        {
            xk[k][n] = 0;
        }
    }
    //se crea la matriz wnk
    for (k = 0; k < N; k++) {
        for (n = 0; n < N; n++) {
            w[n][k][0] = ( *(xn + n) ) * cos((2 * PI*k*n) / N);
            w[n][k][1] = ( *(xn + n) ) * (-sin((2 * PI*k*n) / N));
        }
    }
    // se suma para obtener k
    for (k = 0; k < N; k++)
    {
        for (n = 0; n < N; n++) {
            xk[k][0] += w[n][k][0];
            xk[k][1] += w[n][k][1];
        }
    }
    //se calcula la magnitud
    for (n = 0; n < N; n++) {
        mag[n] = sqrt((xk[n][0])*(xk[n][0]) + (xk[n][1])*(xk[n][1]));
    }
    for(;;){}
}
    //**********************************************************************************************
    //******************************Tratamiento interrupciones**************************************
    //**********************************************************************************************

    //**********************************************************************************************
    //******************************funciones de programa*******************************************
    //**********************************************************************************************


