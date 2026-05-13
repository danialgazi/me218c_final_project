#ifndef THRUSTER_CONTROL_H
#define THRUSTER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>
#include <sys/attribs.h>

#define THRUSTER_CONTROL_JOY_CENTER     127
#define THRUSTER_CONTROL_JOY_MIN        0
#define THRUSTER_CONTROL_JOY_MAX        255

#define THRUSTER_CONTROL_MAX_THRUST     0.50f

void ThrusterControl_Init(void);

void ThrusterControl_SetThrust(uint8_t xJoyStick, uint8_t yJoyStick);

void ThrusterControl_SetRaw(int16_t leftCmd, int16_t rightCmd);

#endif /* THRUSTER_CONTROL_H */