/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef BoatCommsService_H
#define BoatCommsService_H


#include "ES_Types.h"

#define BOAT_COM_JOY_CENTER  127

// Public Function Prototypes

bool InitBoatCommsService(uint8_t Priority);
bool PostBoatCommsService(ES_Event_t ThisEvent);
ES_Event_t RunBoatCommsService(ES_Event_t ThisEvent);

#endif /* BoatCommsService_H */

