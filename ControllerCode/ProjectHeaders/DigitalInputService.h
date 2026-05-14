#ifndef DigitalInputService_H
#define DigitalInputService_H

#include <stdint.h>
#include <stdbool.h>
#include "ES_Events.h"

// Public service functions
bool InitDigitalInputService(uint8_t Priority);
bool PostDigitalInputService(ES_Event_t ThisEvent);
ES_Event_t RunDigitalInputService(ES_Event_t ThisEvent);

// Optional helper functions for other modules
bool DigitalInput_IsShootHeld(void);
bool DigitalInput_IsPairHeld(void);
bool DigitalInput_IsRefuelOn(void);

#endif