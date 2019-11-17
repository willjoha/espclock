#ifndef _CCLOCKDISPLAY_H
#define _CCLOCKDISPLAY_H


#define SMALLCLOCK
//#define BIGCLOCK
//#define BIGCLOCK2


#include "IDisplay.h"
#include <TimeLib.h>             
#include <Timezone.h>             
class CClockDisplay : public IDisplay
{
public:
  CClockDisplay();
  virtual ~CClockDisplay();

  // Color mode for LED colors
  enum eColorMode
  {
	  e_ModeSolid,
	  e_ModeRainbow_1,
	  e_ModeRainbow_2,
	  e_ModeRainbow_3,
	  e_ModeGradient,
	  e_ModeGlitter
  };

  // Dialekt for word
  enum eDialekt
  {
	  e_Bayerisch,
      e_Frankisch,
	  e_Hochdeutsch
  };


  virtual bool setup(CRGB* leds, bool* leds_fill, int numLEDs);
  virtual bool update(bool force=false);

  CRGB getColor();
  void setColor(const CRGB& color);
  void setTimezone(Timezone* pTZ);
  void setColorMode(eColorMode ColorMode);
  eColorMode getColorMode();
  void setDialekt(eDialekt myDialekt);
  eDialekt getDialekt();


private:
  void compose(const int arrayToAdd[]);
  void display_hour(const int displayHour, const int minute, const int hour);
  void display_time(const int hour, const int minute);
  
  CRGB* m_pLEDs;      // actual LEDs with color
  bool* m_pLEDsFill;  // construct LEDs here before applying to m_pLEDs

  int   m_numLEDs;
  CRGB  m_color;
  
  eColorMode m_ColorMode;

  int m_currentMinute;
  Timezone* m_pTZ;

  eDialekt m_Dialekt;
};

#endif
