#include "CClockDisplay.h"

/*
 * Based on https://github.com/thomsmits/wordclock
 * 
 * Copyright (c) 2012 Thomas Smits
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 */

/**
  Positions of the text on the LED panel.
  
  As the Arduino programming language cannot determine
  the length of an array that is passed to a function
  as a pointer, a -1 at the end of the array indicates the
  end of the array.
  
  The values of the array have to be <= NUMBER_OF_LEDS - 1.
*/


const int ES[]      = { 0, 1, -1 };
const int IST[]     = { 3, 4, 5, -1 };
const int FUENF_M[] = { 7, 8, 9, 10, -1 };
const int ZEHN_M[]  = { 18, 19, 20, 21, -1 };
const int ZWANZIG[] = { 11, 12, 13, 14, 15, 16, 17, -1 };
const int DREIVIERTEL[]  = { 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1 };
const int VIERTEL[] = { 26, 27, 28, 29, 30, 31, 32, -1 };
const int VOR[]     = { 35, 36, 37, -1 };
const int FUNK[]    = { 36, 37, 38, 39, -1 };
const int NACH[]    = { 38, 39, 40, 41, -1 };
const int HALB[]    = { 44, 45, 46, 47, -1 };
const int ELF[]     = { 85, 86, 87, -1 };
const int FUENF[]   = { 73, 74, 75, 76, -1 };
const int EIN[]     = { 61, 62, 63, -1 };
const int EINS[]    = { 60, 61, 62, 63, -1 };
const int ZWEI[]    = { 62, 63, 64, 65, -1 };
const int DREI[]    = { 67, 68, 69, 70, -1 };
const int VIER[]    = { 77, 78, 79, 80, -1 };
const int SECHS[]   = { 104, 105, 106, 107, 108, -1 };
const int ACHT[]    = { 89, 90, 91, 92, -1 };
const int SIEBEN[]  = { 55, 56, 57, 58, 59, 60, -1 };
const int ZWOELF[]  = { 49, 50, 51, 52, 53, -1 };
const int ZEHN[]    = { 93, 94, 95, 96, -1 };
const int UHR[]     = { 99, 100, 101, -1 };
const int NEUN[]    = { 81, 82, 83, 84, -1 };

const int SMALL_MIN1[]    = { 112, -1 };
const int SMALL_MIN2[]    = { 112, 113, -1 };
const int SMALL_MIN3[]    = { 112, 113, 114, -1 };
const int SMALL_MIN4[]    = { 112, 113, 114, 115, -1 };

const int BIG_MIN1[] = { 113, -1 };
const int BIG_MIN2[] = { 113, 112, -1 };
const int BIG_MIN3[] = { 113, 112, 111, -1 };
const int BIG_MIN4[] = { 113, 112, 111, 110, -1 };

const int DST[]     = { 109, -1};




CClockDisplay::CClockDisplay() : m_pLEDs(0), m_pLEDsFill(0), m_numLEDs(0), m_color(CRGB::Red), m_currentMinute(-1), m_pTZ(0), m_ColorMode(e_ModeSolid), m_Dialekt(e_Bayerisch), m_smallclock(true)
{

}

CClockDisplay::~CClockDisplay()
{
  
}

bool CClockDisplay::setup(CRGB* leds, bool* leds_fill, int numLEDs)
{
	m_pLEDs = leds;
	m_numLEDs = numLEDs;
	m_pLEDsFill = leds_fill;

	return true;
}

bool CClockDisplay::update(bool force)
{
	if(m_currentMinute != minute() || true == force)
	{
		time_t utc(now());

		// fill complete array with colors, then use composed time to erase not needed

		if (m_ColorMode == e_ModeRainbow_1)
		{
			CHSV colHSV = rgb2hsv_approximate(m_pLEDs[3] );
			fill_rainbow( &(m_pLEDs[0]), m_numLEDs, colHSV.hue, 2);
		}
		else if (m_ColorMode == e_ModeRainbow_2)
		{
			CHSV colHSV = rgb2hsv_approximate(m_pLEDs[3]);
			fill_rainbow( &(m_pLEDs[0]), m_numLEDs, colHSV.hue, 20);
		}
		else if (m_ColorMode == e_ModeRainbow_3)
		{
			CHSV colHSV = rgb2hsv_approximate(m_pLEDs[3]);
			fill_rainbow( &(m_pLEDs[0]), m_numLEDs, colHSV.hue, 40);
		}
		else if (m_ColorMode == e_ModeGradient)
		{
			// inverse color
			CRGB ledcolor;
			ledcolor.r = 255 - m_color.r;
			ledcolor.g = 255 - m_color.g;
			ledcolor.b = 255 - m_color.b;

			fill_gradient_RGB( &(m_pLEDs[0]), m_numLEDs, m_color, ledcolor );
		}
		else if (m_ColorMode == e_ModeGlitter)
		{
			fill_solid(&(m_pLEDs[0]), m_numLEDs, m_color);
		}
		else
		{
			fill_solid(&(m_pLEDs[0]), m_numLEDs, m_color);
		}

		time_t local(0==m_pTZ?utc:m_pTZ->toLocal(utc));

		display_time(hour(local), minute(local));

		m_currentMinute = minute();
		Serial.println();

		//m_pLEDs[0] = m_pLEDs[1];

		return true;
	}

  return false;
}

void CClockDisplay::setDialekt(CClockDisplay::eDialekt myDialekt)
{
	m_Dialekt = myDialekt;
}

CClockDisplay::eDialekt CClockDisplay::getDialekt()
{
	return m_Dialekt;
}

CRGB CClockDisplay::getColor()
{
	return(m_color);
}

void CClockDisplay::setColor(const CRGB& color)
{
	m_color = color;
}

void CClockDisplay::setTimezone(Timezone* pTZ)
{
	m_pTZ = pTZ;
}


void CClockDisplay::setColorMode(CClockDisplay::eColorMode ColorMode)
{
	m_ColorMode = ColorMode;
}

CClockDisplay::eColorMode CClockDisplay::getColorMode()
{
	return m_ColorMode;
}

bool CClockDisplay::isSmallClock(bool isSmallClock)
{
	m_smallclock = isSmallClock;
	return m_smallclock;
}
bool CClockDisplay::isSmallClock()
{
	return m_smallclock;
}

/** 
 Sets the bits in resultArray depending on the values of the 
 arrayToAdd. Bits already set in the input array will not
 be set to zero.
 
 WARNING: this function does not perform any bounds checks!
 
 @param arrayToAdd Array containing the bits to be set as 
        index of the bit. The value 5 for example implies
        that the 6th bit in the resultArray has to be set to 1.
        This array has to be terminated by -1.
 
 @param (out) ledBits Array where the bits will be set according
        to the input array. The ledBits has to be big enough to
        accomodate the indices provided in arrayToAdd.
*/        
void CClockDisplay::compose(const int arrayToAdd[]) 
{
	int pos = 0;
	int i = 0;
    
	while ( (pos = arrayToAdd [i++]) != -1) 
	{
		//m_pLEDs[pos] = m_color;
		m_pLEDsFill[pos] = true;
	}
}

/**
 Sets the hour information for the clock.
 
 @param hour the hour to be set
 @param (out) ledBits array to set led bits
*/
void CClockDisplay::display_hour(const int displayHour, const int minute, const int hour) 
{
  
  int hourAMPM = displayHour;
  
  
	if (displayHour >= 12) 
	{
		hourAMPM -= 12;
	}

	Serial.print(", display_hour: ");
	Serial.print("hour=");
	Serial.print(hour);
	Serial.print(", hourAMPM=");
	Serial.print(hourAMPM);
  
  
  switch (hourAMPM) {
    
    case 0: compose(ZWOELF);
        break;
        
    case 1: 
        if (minute == 0) {
          compose(EIN);
        }
        else {
          compose(EINS);
        }
        break;

    case 2: compose(ZWEI);
        break;
        
    case 3: compose(DREI);
        break;
        
    case 4: compose(VIER);
        break;
        
    case 5: compose(FUENF);
        break;
        
    case 6: compose(SECHS);
        break;
        
    case 7: compose(SIEBEN);
        break;
        
    case 8: compose(ACHT);
        break;
        
    case 9: compose(NEUN);
        break;
        
    case 10: compose(ZEHN);
        break;
        
    case 11: compose(ELF);
        break;
        
    case 12: compose(ZWOELF);
        break;
  }  
}

/**
  Displays hour and minutes on the LED panel.
  
  @param hour the hour to be set
  @param minute the minute to be set
  @param (out) ledBits bits for the LEDs (will NOT be cleared)
*/
void CClockDisplay::display_time(const int hour, const int minute) 
{
	int roundMinute = (minute / 5) * 5;
	int minutesRemaining = minute - roundMinute;

	for (int pos = 0; pos < m_numLEDs; pos++)
	{
		m_pLEDsFill[pos] = false;
	}

	Serial.print("display_time: ");
	Serial.print("Min=");
	Serial.print(roundMinute);
	Serial.print(" minRemain=");
	Serial.print(minutesRemaining);

	compose(ES);
	compose(IST);

	int displayHour = hour;
  
	switch (roundMinute) 
	{
		case 0: 
			compose(UHR);
			Serial.print(", case 0");
			break;
		case 5: 
			compose(FUENF_M);
			compose(NACH);
			Serial.print(", case 5");
			break;
		case 10: 
			compose(ZEHN_M);
			compose(NACH);
			Serial.print(", case 10");
			break;
		case 15: 
			if (m_Dialekt == e_Bayerisch || m_Dialekt == e_Hochdeutsch)
			{
				compose(VIERTEL); 
				compose(NACH);
				Serial.print(", case 15-!nsg");
			}
			else
			{
				compose(VIERTEL); 
				displayHour++;
				Serial.print(", case 15-sg");
			}
			break;
		case 20: 
			compose(ZWANZIG);
			compose(NACH);
			Serial.print(", case 20");
			break;
		case 25: 
			compose(FUENF_M);
			compose(VOR);
			compose(HALB);
			displayHour++;
			Serial.print(", case 25");
			break;
		case 30: 
			compose(HALB);
			displayHour++;
			Serial.print(", case 30");
			break;
		case 35: 
			compose(FUENF_M);
			compose(NACH);
			compose(HALB);
			displayHour++;
			Serial.print(", case 35");
			break;
		case 40: 
			compose(ZWANZIG);
			compose(VOR);
			displayHour++;
			Serial.print(", case 40");
			break;
		case 45: 
			if (m_Dialekt == e_Frankisch || m_Dialekt == e_Bayerisch)
			{
				compose(DREIVIERTEL);
				displayHour++;
				Serial.print(", case 45-sg");
 			}
			else
			{	
				compose(VIERTEL);
				compose(VOR);
				displayHour++;
				Serial.print(", case 45-!sg");
			}
			break;
		case 50: 
			compose(ZEHN_M);
			compose(VOR);
			displayHour++;
			Serial.print(", case 50");
			break;
		case 55: 
			compose(FUENF_M);
			compose(VOR);
			displayHour++;
			Serial.print(", case 55");
			break;
		default:
			Serial.print(", default-case");
	}

	switch (minutesRemaining)
	{
		case 1: compose(m_smallclock ? SMALL_MIN1 : BIG_MIN1);
				Serial.print(", case MIN1");
				break;
		case 2: compose(m_smallclock ? SMALL_MIN2 : BIG_MIN2);
				Serial.print(", case MIN2");
				break;
		case 3: compose(m_smallclock ? SMALL_MIN3 : BIG_MIN3);
				Serial.print(", case MIN3");
				break;
		case 4: compose(m_smallclock ? SMALL_MIN4 : BIG_MIN4);
				Serial.print(", case MIN4");
				break;
		default: break;
	}
  
	display_hour(displayHour, roundMinute, hour);

	for (int pos=0; pos < m_numLEDs; pos++)
	{
		if (!m_pLEDsFill[pos])
		{
			m_pLEDs[pos] = CRGB::Black;
		}
	}

	if (m_ColorMode == e_ModeGlitter)
	{
		m_pLEDs[random8(m_numLEDs)] = CRGB::White;
	}
}
