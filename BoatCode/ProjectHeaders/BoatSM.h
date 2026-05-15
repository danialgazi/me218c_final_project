/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef BoatSM_H
#define BoatSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState, Unpaired, Driving
}BoatState_t;

// Public Function Prototypes

bool InitBoatFSM(uint8_t Priority);
bool PostBoatFSM(ES_Event_t ThisEvent);
ES_Event_t RunBoatFSM(ES_Event_t ThisEvent);
BoatState_t QueryBoatSM(void);

#endif /* BoatSM_H */

