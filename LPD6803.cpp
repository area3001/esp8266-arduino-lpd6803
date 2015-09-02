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

// states
enum lpd6803mode {
  DONE   = 0,
  HEADER = 1,
  DATA   = 2,
  START  = 3
};

#define USE_US_TIMER

extern "C"
{
  #include "ets_sys.h"
  #include "osapi.h"
}

// parameters
static uint16_t s_nr_pixels;
static uint8_t  s_data_pin;
static uint8_t  s_clock_pin;

// the arrays of ints that hold each LED's 15 bit color values
static uint16_t s_p_buffer [64];

// variables
static bool        s_buffer_ready;
static lpd6803mode s_send_mode;    // Used in interrupt 0=start,1=header,2=data,3=done
static byte        s_bit_counter;          // Used in interrupt
static uint16_t    s_led_index;      // Used in interrupt - Which LED we are sending.
static byte        s_pwm_counter;      // Used in interrupt.
static byte        s_last_bit;
static uint16_t    s_begin_pixel_index;   // flag to indicate that the colors need an update asap
static uint16_t    s_end_pixel_index;   // flag to indicate that the colors need an update asap
static uint16_t    swapAsap = 0;   // flag to indicate that the colors need an update asap

// members
static Ticker s_ticker;
static ETSTimer* s_p_timer = 0x0;

//Interrupt routine.
//Frequency was set in begin(). Called once for every bit of data sent
//In your code, set global Sendmode to 0 to re-send the data to the s_p_buffer
//Otherwise it will just send clocks.
void LedOut() {
    switch(s_send_mode) {
        case DONE:            //Done..just send clocks with zero data
            if (swapAsap>0) 
            {
                if(!s_pwm_counter)    //AS SOON AS CURRENT pwm IS DONE. s_pwm_counter
                {
                    s_bit_counter = 0;
                    s_led_index = swapAsap; //set current led
                    s_send_mode = HEADER;
                    swapAsap = 0;
                    s_pwm_counter = 0;
                }
            }
            break;
        case DATA:               //Sending Data
            if ((1 << (15-s_bit_counter)) & s_p_buffer[s_led_index]) 
            {
                if (!s_last_bit) 
                {
                    // digitalwrites take a long time, avoid if possible
                    // If not the first bit then output the next bits
                    // (Starting with MSB bit 15 down.)
                    digitalWrite(s_data_pin, 1);
                    s_last_bit = 1;
                }
            }
            else 
            {
                if (s_last_bit) 
                {
                    // digitalwrites take a long time, avoid if possible
                    digitalWrite(s_data_pin, 0);
                    s_last_bit = 0;
                }
            }
            s_bit_counter++;
            if(s_bit_counter == 16)    //Last bit?
            {
                s_led_index++; //Move to next LED
                if (s_led_index < s_nr_pixels) //Still more leds to go or are we done?
                {
                    s_bit_counter=0; //Start from the fist bit of the next LED
                }
                else 
                {
                    // no longer sending data, set the data pin low
                    digitalWrite(s_data_pin, 0);
                    s_last_bit = 0; // this is a lite optimization
                    s_send_mode = DONE; //No more LEDs to go, we are done!
                }
            }
            break;
        case HEADER:            //Header
            if (s_bit_counter < 32) {
                digitalWrite(s_data_pin, 0);
                s_last_bit = 0;
                s_bit_counter++;
                if (s_bit_counter==32) 
                {
                    s_send_mode = DATA; //If this was the last bit of header then move on to data.
                    s_led_index = 0;
                    s_bit_counter = 0;
                }
            }
            break;
        case START:            //Start
            if (!s_pwm_counter)    //AS SOON AS CURRENT pwm IS DONE. s_pwm_counter
            {
                s_bit_counter = 0;
                s_led_index = 0;
                s_send_mode = HEADER;
            }
            break;
    }

    // Clock out data (or clock LEDs)
    digitalWrite(s_clock_pin, HIGH);
    digitalWrite(s_clock_pin, LOW); 

    //Keep track of where the LEDs are at in their pwm cycle.
    s_pwm_counter++;
}

//---
LPD6803::LPD6803(uint16_t i_nr_pixels, uint8_t i_data_pin, uint8_t i_clock_pin)
{
  // set parameters
  s_data_pin  = i_data_pin;
  s_clock_pin = i_clock_pin;
  s_nr_pixels = i_nr_pixels;
  
  // reset variables
  s_last_bit    = 0;
  //s_bit_counter = 0;
  //s_led_index   = 0;
  //s_pwm_counter = 0;
  //s_send_mode = DONE;

  // allocate pixel buffer
  size_t buffer_size = 2*s_nr_pixels;
  //s_p_buffer = new uint16_t [s_nr_pixels];
  //memset (s_p_buffer, 0, buffer_size*sizeof (uint16_t));
  for (uint16_t i=0; i< buffer_size; i++) 
  {
    setPixelColor(i, 0, 0, 0);
  }
  
  s_send_mode = START;
  s_bit_counter = s_led_index = s_pwm_counter = 0;
}

LPD6803::~LPD6803 ()
{
  // deallocate resources
  //delete [] s_p_buffer;
  //s_p_buffer = 0x0;
}

//---
void LPD6803::begin ()
{
  // set pin modes
  pinMode (s_data_pin, OUTPUT);
  pinMode (s_clock_pin, OUTPUT);
  
  // start timer
  //s_p_timer = new ETSTimer;
  
  //os_timer_setfn(s_p_timer, reinterpret_cast<ETSTimerFunc*>(LedOut), 0x0);
    //ets_timer_arm_new (s_p_timer, 1, 1, 0);
  //s_ticker.attach_ms (1, LedOut);
}

//---
uint16_t LPD6803::getNrPixels ()
{
  return s_nr_pixels;
}

//---
void LPD6803::show (uint16_t i_begin_pixel_index, uint16_t i_end_pixel_index)
{
  s_bit_counter = 0;
  s_led_index = 0;
  s_pwm_counter = 0;
  s_send_mode = START;
  
  s_begin_pixel_index = i_begin_pixel_index;
  s_end_pixel_index   = min (i_end_pixel_index, s_nr_pixels);
  
  s_buffer_ready = true;
}

//---
void LPD6803::doSwapBuffersAsap(uint16_t idx)
{
  swapAsap = idx;
}

//---
void LPD6803::setPixelColor (uint16_t i_pixel_index, uint8_t i_r, uint8_t i_g, uint8_t i_b)
{
  /*if (i_pixel_index >= s_nr_pixels)
  {
    return;
  }
    
  // create color
  uint16_t data;
  data = i_g & 0x1F;
  data <<= 5;
  data |= i_b & 0x1F;
  data <<= 5;
  data |= i_r & 0x1F;
  data |= 0x8000;
  
  // set color
  s_p_buffer [i_pixel_index] = data;*/
  
  color_t color;
  color.r = i_r;
  color.g = i_g;
  color.b = i_b;
  
  setPixelColor (i_pixel_index, color);
}

//---
void LPD6803::setPixelColor (uint16_t i_pixel_index, color_t i_color)
{
  // check boundaries
  if (i_pixel_index >= s_nr_pixels)
  {
    return;
  }
  
  // set color
  i_color.unused = 1;
  s_p_buffer [i_pixel_index] = *reinterpret_cast <uint16_t*> (&i_color);
}

bool LPD6803::outputReady ()
{
  return DONE == s_send_mode;
}

void LPD6803::update ()
{
  LedOut ();
}
