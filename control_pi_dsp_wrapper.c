
/*
 * Include Files
 *
 */
#if defined(MATLAB_MEX_FILE)
#include "tmwtypes.h"
#include "simstruc_types.h"
#else
#define SIMPLIFIED_RTWTYPES_COMPATIBILITY
#include "rtwtypes.h"
#undef SIMPLIFIED_RTWTYPES_COMPATIBILITY
#endif



/* %%%-SFUNWIZ_wrapper_includes_Changes_BEGIN --- EDIT HERE TO _END */
#include <math.h>
// Definimos el tiempo de muestreo
double T = 0.000000002;

// Definimos las ganancias Kp, Ki, Kd
const double Kp = 1;
const double Ki = 1000;
const double kd = 0;

// Definimos las ganancias k1, k2, y k3
double k3;
double k2;
double k1;

// Definimos variables para los errores y salida
double ek2 = 0;
double ek1 = 0;
double ek = 0;
double uk = 0;
double u_aux = 0;
/* %%%-SFUNWIZ_wrapper_includes_Changes_END --- EDIT HERE TO _BEGIN */
#define u_width 1
#define u_1_width 1
#define y_width 1
#define y_1_width 1

/*
 * Create external references here.  
 *
 */
/* %%%-SFUNWIZ_wrapper_externs_Changes_BEGIN --- EDIT HERE TO _END */
/* extern double func(double a); */
/* %%%-SFUNWIZ_wrapper_externs_Changes_END --- EDIT HERE TO _BEGIN */

/*
 * Start function
 *
 */
void control_pi_dsp_Start_wrapper(void)
{
/* %%%-SFUNWIZ_wrapper_Start_Changes_BEGIN --- EDIT HERE TO _END */
// Inicializamos k1, k2, y k3 dentro de la función de inicio
    k3 = kd;
    k2 = -1 * (Kp * T + 2 * kd);
    k1 = (Kp * T + Ki * T * T + kd);
/* %%%-SFUNWIZ_wrapper_Start_Changes_END --- EDIT HERE TO _BEGIN */
}
/*
 * Output function
 *
 */
void control_pi_dsp_Outputs_wrapper(const real_T *Vref,
			const real_T *Vmeas,
			real_T *y0,
			real_T *error)
{
/* %%%-SFUNWIZ_wrapper_Outputs_Changes_BEGIN --- EDIT HERE TO _END */
//definimos el error
    ek2 = ek1;
    ek1 = ek;
    ek = Vref[0]-Vmeas[0];
 
    
    u_aux = uk + (1/T)*(k1*ek+k2*ek1+k3*ek2);

    //definimos los limites superiors e inferiores

   if(u_aux < 0){
    uk = 0;  // Si u_aux es menor a 0, fija uk a 0
} else if(u_aux > 3.3){
    uk = 3.3;  // Si u_aux es mayor a 3, fija uk a 3
} else {
    uk = u_aux;  // Si está dentro del rango, asigna u_aux a uk
}

    y0[0] = uk;
    error[0] = ek;
/* %%%-SFUNWIZ_wrapper_Outputs_Changes_END --- EDIT HERE TO _BEGIN */
}


