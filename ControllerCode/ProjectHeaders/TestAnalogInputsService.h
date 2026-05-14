/****************************************************************************

  Header file for TestAnalogInputs service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServTestAnalogInputs_H
#define ServTestAnalogInputs_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitTestAnalogInputsService(uint8_t Priority);
bool PostTestAnalogInputsService(ES_Event_t ThisEvent);
ES_Event_t RunTestAnalogInputsService(ES_Event_t ThisEvent);

#endif /* ServTestAnalogInputs_H */

