#ifndef _ISYNCPROVIDER_H
#define _ISYNCPROVIDER_H

#include <TimeLib.h>

class ISyncProvider
{
  public:
  virtual ~ISyncProvider() {};

  virtual time_t now() = 0;
};

#endif
