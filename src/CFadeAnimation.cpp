#include "CFadeAnimation.h"

CFadeAnimation::CFadeAnimation(): previousMillis(0), interval(30) {}

CFadeAnimation::~CFadeAnimation() {}

const uint8_t fadespeed[256] = {  1,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                                 20, 21, 22, 23, 24, 25, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
                                 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 53, 54, 55, 56,
                                 57, 58, 59, 60, 61, 62, 62, 63, 64, 65, 66, 67, 68, 69, 69, 70, 71, 72, 73, 74,
                                 75, 75, 76, 77, 78, 79, 80, 80, 81, 82, 83, 84, 85, 85, 86, 87, 88, 89, 89, 90,
                                 91, 92, 93, 93, 94, 95, 96, 96, 97, 98, 99,100,100,101,102,103,103,104,105,106,
                                106,107,108,108,109,110,111,111,112,113,113,114,115,115,116,117,117,118,119,119,
                                120,121,121,122,122,123,124,124,125,125,126,127,127,128,128,129,129,130,130,131,
                                131,132,133,133,134,134,134,135,135,136,136,137,137,138,138,138,139,139,140,140,
                                140,141,141,141,142,142,142,142,143,143,143,143,144,144,144,144,144,144,144,145,
                                145,145,145,145,145,145,145,145,145,144,144,144,144,144,143,143,143,142,142,142,
                                141,141,140,139,139,138,137,137,136,135,134,132,131,130,129,127,125,124,122,120,
                                117,115,112,109,106,102, 98, 93, 87, 81, 73, 63, 50, 32, 16,  8};


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

