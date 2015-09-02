#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class LPD6803
{
public:

#pragma pack (push, 1)
  struct color_t
  {
    unsigned b : 5;
    unsigned g : 5;
    unsigned r : 5;
    unsigned unused : 1;
  };
#pragma pack (pop)
  
  
  LPD6803 (uint16_t i_nr_pixels, uint8_t i_data_pin, uint8_t i_clock_pin);
  ~LPD6803 ();
  void begin ();
  void show (uint16_t i_begin_pixel_index=0, uint16_t i_end_pixel_index=65535);
  void doSwapBuffersAsap(uint16_t idx);
  void setPixelColor (uint16_t i_pixel_index, uint8_t i_r, uint8_t i_g, uint8_t i_b);
  void setPixelColor (uint16_t i_pixel_index, color_t i_color);
  uint16_t getNrPixels ();
  bool outputReady ();
  
  void update ();
  
private:

};
