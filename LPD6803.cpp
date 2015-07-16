#include <Ticker.h>
#include "LPD6803.h"
/*****************************************************************************
* Example to control LPD6803-based RGB LED Modules in a strand
* Original code by Bliptronics.com Ben Moyes 2009
* Use this as you wish, but please give credit, or at least buy some of my LEDs!
*
* Code cleaned up and Object-ified by ladyada, should be a bit easier to use
*
* Library Optimized for fast refresh rates 2011 by michu@neophob.com
*****************************************************************************/
// the arrays of ints that hold each LED's 15 bit color values
static uint16_t *pixels;
static uint16_t numLEDs;
static uint8_t dataPin, clockPin;
enum lpd6803mode {
    START,
    HEADER,
    DATA,
    DONE
};
static lpd6803mode SendMode;    // Used in interrupt 0=start,1=header,2=data,3=done
static byte  BitCount;          // Used in interrupt
static uint16_t  LedIndex;      // Used in interrupt - Which LED we are sending.
static byte  BlankCounter;      // Used in interrupt.
static byte lastdata = 0;
static uint16_t swapAsap = 0;   // flag to indicate that the colors need an update asap
static Ticker ticker;

//Interrupt routine.
//Frequency was set in begin(). Called once for every bit of data sent
//In your code, set global Sendmode to 0 to re-send the data to the pixels
//Otherwise it will just send clocks.
void LedOut() {
    switch(SendMode) {
        case DONE:            //Done..just send clocks with zero data
            if (swapAsap>0) 
            {
                if(!BlankCounter)    //AS SOON AS CURRENT pwm IS DONE. BlankCounter
                {
                    BitCount = 0;
                    LedIndex = swapAsap; //set current led
                    SendMode = HEADER;
                    swapAsap = 0;
                    BlankCounter = 0;
                }
            }
            break;
        case DATA:               //Sending Data
            if ((1 << (15-BitCount)) & pixels[LedIndex]) 
            {
                if (!lastdata) 
                {
                    // digitalwrites take a long time, avoid if possible
                    // If not the first bit then output the next bits
                    // (Starting with MSB bit 15 down.)
                    digitalWrite(dataPin, 1);
                    lastdata = 1;
                }
            }
            else 
            {
                if (lastdata) 
                {
                    // digitalwrites take a long time, avoid if possible
                    digitalWrite(dataPin, 0);
                    lastdata = 0;
                }
            }
            BitCount++;
            if(BitCount == 16)    //Last bit?
            {
                LedIndex++; //Move to next LED
                if (LedIndex < numLEDs) //Still more leds to go or are we done?
                {
                    BitCount=0; //Start from the fist bit of the next LED
                }
                else 
                {
                    // no longer sending data, set the data pin low
                    digitalWrite(dataPin, 0);
                    lastdata = 0; // this is a lite optimization
                    SendMode = DONE; //No more LEDs to go, we are done!
                }
            }
            break;
        case HEADER:            //Header
            if (BitCount < 32) {
                digitalWrite(dataPin, 0);
                lastdata = 0;
                BitCount++;
                if (BitCount==32) 
                {
                    SendMode = DATA; //If this was the last bit of header then move on to data.
                    LedIndex = 0;
                    BitCount = 0;
                }
            }
            break;
        case START:            //Start
            if (!BlankCounter)    //AS SOON AS CURRENT pwm IS DONE. BlankCounter
            {
                BitCount = 0;
                LedIndex = 0;
                SendMode = HEADER;
            }
            break;
    }

    // Clock out data (or clock LEDs)
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW); 

    //Keep track of where the LEDs are at in their pwm cycle.
    BlankCounter++;
}

//---
LPD6803::LPD6803(uint16_t n, uint8_t dpin, uint8_t cpin) {
    dataPin = dpin;
    clockPin = cpin;
    numLEDs = n;

    pixels = (uint16_t *)malloc(numLEDs);

    for (uint16_t i=0; i< numLEDs; i++) 
    {
        setPixelColor(i, 0, 0, 0);
    }
    SendMode = START;
    BitCount = LedIndex = BlankCounter = 0;
}

//---
void LPD6803::begin(void) {
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    ticker.attach_ms(4, LedOut);
}

//---
uint16_t LPD6803::numPixels(void) {
    return numLEDs;
}

//---
void LPD6803::show(void) {
    BitCount = LedIndex = BlankCounter = 0;
    SendMode = START;
}

//---
void LPD6803::doSwapBuffersAsap(uint16_t idx) {
    swapAsap = idx;
}

//---
void LPD6803::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
    if (n > numLEDs) return;
    uint16_t data;
    data = g & 0x1F;
    data <<= 5;
    data |= b & 0x1F;
    data <<= 5;
    data |= r & 0x1F;
    data |= 0x8000;
    pixels[n] = data;
}

//---
void LPD6803::setPixelColor(uint16_t n, uint16_t c) {
    if (n > numLEDs) return;
    pixels[n] = 0x8000 | c;
}