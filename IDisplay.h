#ifndef _IDISPLAY_H
#define _IDIDPLAY_H

#include <FastLED.h>

class IDisplay
{
  public:

  virtual ~IDisplay() {};

  virtual bool setup(CRGB* leds, int numLEDs) = 0;

  virtual bool update(bool force=false) = 0;
  
};

#endif
