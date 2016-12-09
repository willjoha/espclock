#ifndef _CNTPCLIENT_H
#define _CNTPCLIENT_H

#include "ISyncProvider.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

class CNTPClient : public ISyncProvider
{
public:
  CNTPClient();
  virtual ~CNTPClient();

  bool setup(IPAddress timeServer);
  void setTimeServer(IPAddress timeServer);

  virtual time_t now();
  
private:
  void sendNTPpacket(IPAddress &address);
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

  WiFiUDP Udp;
  IPAddress m_timeServer;
};


#endif
