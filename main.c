#include <mcs51/lint.h>
#include <mcs51/8051.h>
#include "stc15w.h"
#include "delay.h"
#include "uart.h"
#include "dip.h"
#include "config.h"
#include "leds.h"

//400 interrupts/s 
#define STROBE_TIMER_START (65536-FOSC/12/400)
// on time of a strobe flash in 2.5ms steps
#define STROBE_ON_TIME_MS 4


extern volatile unsigned char dmxData[NUM_ADRESSES]; //defined in uart.c
extern unsigned short dmxAddr; //defined in uart.c
unsigned char functionBit = 0;
extern unsigned char ledBrightness[NUM_LEDS]; //defined in leds.c

/** this value is increased by a timer every 2.5ms.
 *  It is used in the strobe logic.
 *  The strobe logic also resets this value to zero every time a strobe flash finishes.
 *  The unit of this is 2.5ms thus the maximum delay the timer can achieve is 
 *  637ms */
volatile unsigned char timeMs = 0;

void initStrobeTimer()
{
    AUXR &= 0xBF;   // Set timer1 clock source to sysclk/12 (12T mode)
    TMOD &= 0x0F;    // Clear 4bit field for timer1

    //set reload counter
    TH1 = STROBE_TIMER_START >> 8;
    TL1 = (unsigned char)STROBE_TIMER_START;

    // set timer 1 interrupt priority to low
    PT1 = 0; 

    TR1 = 1; // Start Timer 1

    //enable interrupt
    ET1 = 1;
    EA  = 1;
}


void strobeTimerInterrupt()  __interrupt(TF1_VECTOR) __using(1)
{
    // has the same priority as uart.
    // thus we can do nearly nothing in here. 
    ++timeMs;
}


inline unsigned char calcStrobeTimeMs(unsigned char strobeDmxVal)
{
    // maps between 200 and 10 timer ticks
    // i.e. 500ms and 25ms delay
    // i.e. 2hz and 40hz

    /** 
     * Unoptimized floating point version of this function:
     * 
     * long map(long x, long in_min, long in_max, long out_min, long out_max) 
     * {
     *     return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
     * }
     * return map(strobeDmxVal, 0, 255, 255, 25);
     * 
     * This code has been optimized by using fixed comma math with factor 255.
     */

    
    return (51000 - strobeDmxVal * 190)/255;

}

void flickerPwrLed()
{
    static unsigned char pwrLedCnt = 0;
    //if power led has been turned off by uart, leave it off for 255 loop iterations
    if(P0_3)
    {
        pwrLedCnt++;
        if(pwrLedCnt == 255)
        {
            //turn on power led 
            //uart turns it off when it has received a correct frame.
            //this results in flickering if dmx is present and steady on if no dmx present.
            P0_3 = 0;
            pwrLedCnt = 0;
        }
    }
}

inline void readDipSwitch()
{
    dmxAddr = readDmxAddr();
 
    if(dmxAddr == 0)
    {
        dmxAddr = 1;
    }
    if(dmxAddr > 512 - NUM_ADRESSES)
    {
        dmxAddr = 512 - NUM_ADRESSES;
    }

    functionBit = readFunctionDip();
}


void main()
{
    unsigned short masterBrightness = 0;
    unsigned char strobeDmx = 0;
    unsigned char strobeOffTime = 0;
    
    unsigned char oldStrobe = 0;
    unsigned char strobeOn = 0; //current state of strobe (led on or off)
    

    dipInit();

    readDipSwitch();

    uartInit(); //initially sets AUXR
    ledInit(); //modifies AUXR
    initStrobeTimer(); //modifes AUXR

    P0_3 = 0; //turn on power

    while(1)
    {
        //the sdcc does not inline this, if we need more performance inline manually
        flickerPwrLed();
        readDipSwitch();


        strobeDmx = dmxData[1];
        if(!oldStrobe && strobeDmx)
        {
            //strobe has been turned on, reset strobe start time to now
            timeMs = 0;
            TH1 = STROBE_TIMER_START >> 8;
            TL1 = (unsigned char)STROBE_TIMER_START;
            strobeOn = 1;
        }
        oldStrobe = strobeDmx;

        if(strobeDmx)
        {
            if(strobeOn)
            {
                //check if strobe flash needs to be turned off
                if(timeMs >=  STROBE_ON_TIME_MS)
                {
                    //led was on long enough, turn off
                    masterBrightness = 0;
                    strobeOn = 0;
                    timeMs = 0;
                }
                else
                {
                    //led should stay on (or turn on)
                    masterBrightness = dmxData[0];
                }
            }
            else
            {
                strobeOffTime = calcStrobeTimeMs(strobeDmx);
                //check if it is time to turn on strobe
                if(timeMs > strobeOffTime)
                {
                    //time to turn the strobe back on
                    masterBrightness = dmxData[0];
                    strobeOn = 1;
                    timeMs = 0;
                }
                else
                {
                    //leave it off a little longer (or turn it off)
                    masterBrightness = 0;
                }
            }
        }
        else
        {
            masterBrightness = dmxData[0];
        }


        // The master scaling is done in fixed point math with scale 255
        // 255 was chosen because it allows to ommit scaling of masterBrightness
        // and is close to the theoretical maximum scale of 257 
        // (255*257=biggest possible unsigned short).

        //loop unrolled for performance reasons
        ledBrightness[0] = (dmxData[2] * masterBrightness) / 255;
        ledBrightness[1] = (dmxData[3] * masterBrightness) / 255;
        ledBrightness[2] = (dmxData[4] * masterBrightness) / 255;
        ledBrightness[3] = (dmxData[5] * masterBrightness) / 255;
        ledBrightness[4] = (dmxData[6] * masterBrightness) / 255;
        ledBrightness[5] = (dmxData[7] * masterBrightness) / 255;
        ledBrightness[6] = (dmxData[8] * masterBrightness) / 255;
        ledBrightness[7] = (dmxData[9] * masterBrightness) / 255;

        // for(i = 2; i < NUM_LEDS + 2; ++i)
        // {
        //     ledBrightness[i-2] = (dmxData[i] * masterBrightness) / 255;
        // }
    }
}
