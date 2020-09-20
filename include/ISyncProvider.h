#ifndef _ISYNCPROVIDER_H
#define _ISYNCPROVIDER_H

#include <TimeLib.h>              // http://www.arduino.cc/playground/Code/Time Time, by Michael Margolis includes
                                  // https://github.com/PaulStoffregen/Time V1.5 

class ISyncProvider
{
  public:
  virtual ~ISyncProvider() {};

  virtual time_t now() = 0;
};

#endif
