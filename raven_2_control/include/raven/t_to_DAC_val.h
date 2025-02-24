/*
 * t_to_DAC_val.h
 *
 * Kenneth Fodero
 * Biorobotics Lab
 * 2005
 *
 */

//#include <rtai.h>

//Local include files
#include "struct.h" /*Includes DS0, DS1, DOF_type*/

//These are not used, but kept in for reference
//#define DAC_MAX_V   10 /*  10V max on DAC */
//#define DAC_MIN_V  -10 /* -10V min on DAC */
//#define TF_DAC  ((float)2*DAC_MAX_V/(float)DAC_STEPS) // (DAC_MAX_V-DAC_MIN_V)/DAC_STEPS = Volts / Dac_increment

#define DAC_STEPS  65536 /* 2^16 steps avail */


//Wrappers for c++
#ifdef __cplusplus
 extern "C" {
 #endif

short int tToDACVal(struct DOF *joint);
void clearDACs(struct device *device0);

int TorqueToDAC(struct device *device0);
int TorqueToDACTest(struct device *device0); //Square wave for timing test

 #ifdef __cplusplus
 }
 #endif
