/****************************************************************************
 Module
   WS2812.c

 Description
   Bit-banged WS2812 (NeoPixel) driver for PIC32 (cycle-accurate)

 Author
   Ege Turan with some modifications from Daniel
****************************************************************************/

#include "WS2812.h"
#include <xc.h>

#define MAX_LEDS 1
#define WS_PIN LATAbits.LATA4
#define WS_TRIS TRISAbits.TRISA4

static uint8_t leds_buffer[MAX_LEDS * 3]; // LED data (G,R,B)


#define SEND_0_BIT() do {                         \
    WS_PIN = 1;                                   \
    __asm__ volatile (                            \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
    );                                            \
    WS_PIN = 0;                                   \
    __asm__ volatile (                            \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
    );                                            \
} while (0)

#define SEND_1_BIT() do {                         \
    WS_PIN = 1;                                   \
    __asm__ volatile (                            \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
    );                                            \
    WS_PIN = 0;                                   \
    __asm__ volatile (                            \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n nop\n nop\n"                 \
        "nop\n nop\n"                             \
    );                                            \
} while (0)

#define SEND_BYTE(color) do {                                      \
    uint8_t c = (color);                                           \
    if (c & 0x80) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x40) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x20) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x10) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x08) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x04) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x02) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
    if (c & 0x01) { SEND_1_BIT(); } else { SEND_0_BIT(); }         \
} while (0)
/****************************************************************************
// Initialization
****************************************************************************/
void neopixel_init(void)
{
    WS_TRIS = 0;  // Output
    WS_PIN = 0;
}

/****************************************************************************
// Send single WS2812 bit using tight NOP loops
// Timing for 40MHz PBCLK (1 tick = 25ns)
// '0': T_H ~ 0.35us, T_L ~ 0.8us
// '1': T_H ~ 0.7us,  T_L ~ 0.6us
// Caution: blocking code
// Note: Hard-coded according to oscilloscope measurements. Tested and verified.
****************************************************************************/

/****************************************************************************
// Update LEDs
****************************************************************************/
void neopixel_show(void)
{
    __builtin_disable_interrupts();

    for (int i = 0; i < MAX_LEDS; i++)
    {
        SEND_BYTE(leds_buffer[i*3 + 0]); // G
        SEND_BYTE(leds_buffer[i*3 + 1]); // R
        SEND_BYTE(leds_buffer[i*3 + 2]); // B
    }

    __builtin_enable_interrupts();

    // Reset >50us
    WS_PIN = 0;
    for (volatile int i = 0; i < 2000; i++) {
        __asm__ volatile ("nop");
    }
}

/****************************************************************************
// Public: Set pixel color
****************************************************************************/
void neopixel_set_pixel(int i, uint8_t r, uint8_t g, uint8_t b)
{
    if (i < 0 || i >= MAX_LEDS) return;
    leds_buffer[i*3 + 0] = g;
    leds_buffer[i*3 + 1] = r;
    leds_buffer[i*3 + 2] = b;
}

/****************************************************************************
// Public: Clear all LEDs
****************************************************************************/
void neopixel_clear(void)
{
    for (int i = 0; i < MAX_LEDS*3; i++) leds_buffer[i] = 0;
}
