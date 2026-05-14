/****************************************************************************
 Module
   ThrusterModule.c

 Revision
   1.0.0

 Description
  Interprets raw Thruster ADC values from ADService into thruster power 
  information for the motors.

 History
 When           Who     What/Why
 -------------- ---     --------
 05/10/26       DA      scaffolding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ThrusterModule.h"
#include "ADService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define JOYSTICK_RANGE 1023
#define MAX_THRUST 0xFF         // full forward
#define MIN_THRUST 0x00         // full reverse
#define MAX_STEER_RIGHT    0xFF
#define MAX_STEER_LEFT     0x00
/*---------------------------- Module Variables ---------------------------*/
static uint8_t thrusterPowerByte;
static uint8_t steeringPowerByte;
/*------------------------------ Module Code ------------------------------*/

/*
  Converts joystick x and y values into bytes for steering and power as
  specified in the class communications protocol.
*/
void ThrusterModule_EncodeThrusterPower(void)
{
  uint32_t joystickX = getJoystickX();    // X = steering (joy2)
  uint32_t joystickY = getJoystickY();    // Y = thrust (joy1)

  steeringPowerByte = joystickX * (MAX_STEER_RIGHT - MAX_STEER_LEFT) / JOYSTICK_RANGE;
  thrusterPowerByte = joystickY * (MAX_THRUST - MIN_THRUST) / JOYSTICK_RANGE;
}

/* 
  Returns thruster power byte, from 0x00 (full reverse) to 0xFF (full forward).
  0x7F = stationary.
*/
uint8_t ThrusterModule_GetJoy1(void)
{
  return thrusterPowerByte;
}

/*
  Returns steering power byte, from 0x00 (full left) to 0xFF (full right). 
  0x7F = stationary.
*/
uint8_t ThrusterModule_GetJoy2(void)
{
  return steeringPowerByte;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
