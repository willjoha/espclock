#ifndef _IANIMATION_H
#define _IANIMATION_H

#include <FastLED.h>

class IAnimation
{
  public:
  virtual ~IAnimation() {};

  virtual bool transform(CRGB* current, CRGB* target, int num_leds, bool changed) = 0;

  
};

#endif
