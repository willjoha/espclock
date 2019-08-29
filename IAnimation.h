#ifndef _IANIMATION_H
#define _IANIMATION_H

#include <FastLED.h>   // https://github.com/FastLED/FastLED v3.3.0
 
class IAnimation
{
  public:
  virtual ~IAnimation() {};

  virtual bool transform(CRGB* current, CRGB* target, int num_leds, bool changed) = 0;
  
};

#endif
