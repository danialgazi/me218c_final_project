/****************************************************************************
 Template header file for Neopixel Module

 ****************************************************************************/

#ifndef NEOPIXEL_MODULE_H
#define NEOPIXEL_MODULE_H

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 Public Functions
****************************************************************************/

void NeopixelModule_Init(void);

void LEDS_Unpaired(void);
void LEDS_Pairing(void);
void LEDS_Driving(void);
void LEDS_Refueling(void);


#endif