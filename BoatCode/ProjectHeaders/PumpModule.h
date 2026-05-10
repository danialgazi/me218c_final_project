#ifndef PUMP_MODULE_H
#define PUMP_MODULE_H

#include <stdbool.h>

void Pump_Init(void);
void Pump_On(void);
void Pump_Off(void);
bool Pump_IsOn(void);

#endif
