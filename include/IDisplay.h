#ifndef _IDISPLAY_H
#define _IDIDPLAY_H

#include <FastLED.h> // https://github.com/FastLED/FastLED v3.3.0

class IDisplay
{
  public:

  virtual ~IDisplay() {};

  virtual bool setup(CRGB* leds, bool* leds_fill, int numLEDs) = 0;

  virtual bool update(bool force=false) = 0;
  
};

#endif
