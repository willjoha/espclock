#include "CFadeAnimation.h"

CFadeAnimation::CFadeAnimation(): previousMillis(0), interval(30) {}

CFadeAnimation::~CFadeAnimation() {}


const uint8_t mylog[68] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 27,
30, 33, 36, 39, 42, 46, 49, 53, 56, 60, 64, 68, 72, 77, 81, 86, 90, 95, 100, 105, 110, 116,
121, 127, 132, 138, 144, 150, 156, 163, 169, 176, 182, 189, 196, 203, 210, 218, 225,
233, 240, 248, 255};


bool CFadeAnimation::transform(CRGB* current, CRGB* target, int num_leds, bool changed)
{
	bool bchanged(false);

	static uint8_t n = 0;

	if(changed) 
		n = 0;

	unsigned long currentMillis = millis();
	if(currentMillis - previousMillis >= interval)
	{
		previousMillis = currentMillis;
		for(int i = 0; i < num_leds; i++)
		{
			if(current[i] != target[i])
			{
				current[i].r = sqrt16(scale16by8( current[i].r * current[i].r, (255-mylog[n]) ) + scale16by8( target[i].r * target[i].r, mylog[n] ));
				current[i].g = sqrt16(scale16by8( current[i].g * current[i].g, (255-mylog[n]) ) + scale16by8( target[i].g * target[i].g, mylog[n] ));
				current[i].b = sqrt16(scale16by8( current[i].b * current[i].b, (255-mylog[n]) ) + scale16by8( target[i].b * target[i].b, mylog[n] ));
				bchanged = true;
			}
		}
		if(!bchanged) 
			n = 0;
		else if(n < 63) 
			n++;
	}

	return(bchanged);
}

bool CFadeAnimation::transform2(CRGB* current, CRGB* target, int num_leds)
{
	for (int i = 0; i < num_leds; i++)
	{
		current[i] = target[i];
	}

	return true;
}

