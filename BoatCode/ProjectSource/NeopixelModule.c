/****************************************************************************
 Module
   NeoPixel Module

 Description
   Higher Level Logic using neopixels, specific to this boat.

 Author
   Daniel Rullan
****************************************************************************/

#include "NeopixelModule.h"
#include "WS2812.h"
#include <xc.h>
#include <stdint.h>

/*----------------------------- Module Defines ----------------------------*/

#define NUM_LEDS 10

// Keep colors dim to reduce current draw.
// WS2812s can get very bright/current-hungry at full brightness.
#define DIM_RED_R       30
#define DIM_RED_G       0
#define DIM_RED_B       0

#define DIM_GREEN_R     0
#define DIM_GREEN_G     30
#define DIM_GREEN_B     0

#define DIM_BLUE_R      0
#define DIM_BLUE_G      0
#define DIM_BLUE_B      30

#define DIM_YELLOW_R    30
#define DIM_YELLOW_G    20
#define DIM_YELLOW_B    0

#define DIM_PURPLE_R    25
#define DIM_PURPLE_G    0
#define DIM_PURPLE_B    25

#define DIM_WHITE_R     20
#define DIM_WHITE_G     20
#define DIM_WHITE_B     20

#define LED_OFF_R       0
#define LED_OFF_G       0
#define LED_OFF_B       0

/*---------------------------- Private Functions --------------------------*/

static void SetAllLEDs(uint8_t r, uint8_t g, uint8_t b);
static void ShowStaticAlternating(uint8_t r1, uint8_t g1, uint8_t b1,
                                  uint8_t r2, uint8_t g2, uint8_t b2);

/****************************************************************************
 Function
   NeopixelModule_Init

 Description
   Initializes the low-level NeoPixel driver and turns all LEDs off.
****************************************************************************/
void NeopixelModule_Init(void)
{
    neopixel_init();
    neopixel_clear();
    neopixel_show();
}

/****************************************************************************
 Function
   LEDS_Unpaired

 Description
   Boat is not paired with a controller.
   Pattern: solid dim red.
****************************************************************************/
void LEDS_Unpaired(void)
{
    SetAllLEDs(DIM_RED_R, DIM_RED_G, DIM_RED_B);
}

/****************************************************************************
 Function
   LEDS_Pairing

 Description
   Boat is currently pairing / waiting for pairing confirmation.
   Pattern: solid dim yellow.
****************************************************************************/
void LEDS_Pairing(void)
{
    SetAllLEDs(DIM_YELLOW_R, DIM_YELLOW_G, DIM_YELLOW_B);
}

/****************************************************************************
 Function
   LEDS_Gameplay

 Description
   Boat is paired and actively in gameplay mode.
   Pattern: solid dim green.
****************************************************************************/
void LEDS_Gameplay(void)
{
    SetAllLEDs(DIM_GREEN_R, DIM_GREEN_G, DIM_GREEN_B);
}

/****************************************************************************
 Function
   LEDS_Refueling

 Description
   Boat is in refueling / steam recovery mode.
   Pattern: solid dim blue.
****************************************************************************/
void LEDS_Refueling(void)
{
    SetAllLEDs(DIM_BLUE_R, DIM_BLUE_G, DIM_BLUE_B);
}

/****************************************************************************
 Function
   LEDS_CommsLost

 Description
   Boat lost communication with controller.
   Pattern: static alternating red/purple.
   This is not animated, so it does not block repeatedly.
****************************************************************************/
void LEDS_CommsLost(void)
{
    SetAllLEDs(DIM_PURPLE_R, DIM_PURPLE_G, DIM_PURPLE_B);
}

/***************************************************************************
 Private Functions
 ***************************************************************************/

static void SetAllLEDs(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        neopixel_set_pixel(i, r, g, b);
    }

    neopixel_show();
}

static void ShowStaticAlternating(uint8_t r1, uint8_t g1, uint8_t b1,
                                  uint8_t r2, uint8_t g2, uint8_t b2)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if ((i % 2) == 0)
        {
            neopixel_set_pixel(i, r1, g1, b1);
        }
        else
        {
            neopixel_set_pixel(i, r2, g2, b2);
        }
    }

    neopixel_show();
}