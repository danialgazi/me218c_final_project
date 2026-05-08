/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef CommsService_H
#define CommsService_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitCommsService(uint8_t Priority);
bool PostCommsService(ES_Event_t ThisEvent);
ES_Event_t RunCommsService(ES_Event_t ThisEvent);

#endif /* CommsService_H */

