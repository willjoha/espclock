#include <FS.h>                   //From the WifiManager AutoConnectWithFSParameters: this needs to be first, or it all crashes and burns... hmmm is this really the case?

#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <TimeLib.h>              //http://www.arduino.cc/playground/Code/Time
#include "CRTC.h"
#include "CNTPClient.h"
#include <Timezone.h>             //https://github.com/JChristensen/Timezone (Use https://github.com/willjoha/Timezone as long as https://github.com/JChristensen/Timezone/pull/8 is not merged.)

#include <FastLED.h>
#include "CFadeAnimation.h"

#include "CClockDisplay.h"

#include <ESP8266WiFi.h>

#include <DNSServer.h>            
#include <ESP8266WebServer.h>     
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <Ticker.h>
Ticker ticker;

#define BLYNK_PRINT Serial        // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>

#include "OpcServer.h"            //https://github.com/plasticrake/OpcServer (use a version which includes the following commit: https://github.com/plasticrake/OpcServer/commit/32eafd7d13799ca23b570ec2547ff225f01c015d)

/*
 * ------------------------------------------------------------------------------
 * LED configuration
 * ------------------------------------------------------------------------------
 */

// Number of LEDs used for the clock (11x10 + 4 minute LEDs + 4 spare LEDs)
#ifdef SMALLCLOCK
#define NUM_LEDS 118
#else
#define NUM_LEDS 122
#endif

// DATA_PIN and CLOCK_PIN used by FastLED to control the APA102C LED strip. 
#define DATA_PIN 13
#define CLOCK_PIN 14

CRGB leds[NUM_LEDS];
CRGB leds_target[NUM_LEDS];

/*
 * ------------------------------------------------------------------------------
 * Open Pixel Control Server configuration
 * ------------------------------------------------------------------------------
 */
 
const int OPC_PORT = 7890;
const int OPC_CHANNEL = 1;      // Channel to respond to in addition to 0 (broadcast channel)
const int OPC_MAX_CLIENTS = 2;  // Maxiumum Number of Simultaneous OPC Clients
const int OPC_MAX_PIXELS = NUM_LEDS;
const int OPC_BUFFER_SIZE = OPC_MAX_PIXELS * 3 + OPC_HEADER_BYTES;

/*
 * ------------------------------------------------------------------------------
 * Configuration parameters configured by the WiFiManager and stored in the FS
 * ------------------------------------------------------------------------------
 */
 
//The default values for the NTP server and the Blynk token, if there are different values in config.json, they are overwritten.
char ntp_server[50] = "0.de.pool.ntp.org";
char blynk_token[33] = "YOUR_BLYNK_TOKEN";

/*
 * ------------------------------------------------------------------------------
 * Clock configuration/variables
 * ------------------------------------------------------------------------------
 */

uint8_t brightnessNight =  5;
uint8_t brightnessDay   = 50;
uint8_t brightness = brightnessDay;

bool displayClock = true;


CRTC Rtc;
CNTPClient Ntp;

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

CFadeAnimation ani;
CClockDisplay clock;

void updateBrightness(bool force=false)
{
  static bool bDay=true;
  
  if(true == force)
  {
    brightness = brightnessDay;
    Blynk.virtualWrite(V1, brightness);
    FastLED.setBrightness(brightness);
    bDay = true;  
  }
  
  
  time_t local(CE.toLocal(now()));
  if(hour(local) >= 22 || hour(local) < 6)
  {
    if(true == bDay)
    {
      Serial.println("Updating the brightness for the night.");
      brightness = brightnessNight;
      Blynk.virtualWrite(V1, brightness);
      FastLED.setBrightness(brightness);
      bDay = false;  
    }
  }
  else if (false == bDay)
  {
    Serial.println("Updating the brightness for the day.");
    brightness = brightnessDay;
    Blynk.virtualWrite(V1, brightness);
    FastLED.setBrightness(brightness);
    bDay = true;
  }
}

/*
 * ------------------------------------------------------------------------------
 * Callbacks
 * ------------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
//Time callback:

time_t getDateTimeFromRTC()
{
  return Rtc.now();
}

//------------------------------------------------------------------------------
//Blynk callbacks:

BLYNK_WRITE(V0)
{
  CRGB ledcolor;
  ledcolor.r = param[0].asInt();
  ledcolor.g = param[1].asInt();
  ledcolor.b = param[2].asInt();
  clock.setColor(ledcolor);
  clock.update(true);
  //Blynk.virtualWrite(V0, ledcolor.r, ledcolor.g, ledcolor.b);
}

//BLYNK_READ(V0)
//{
//  CRGB ledcolor(clock.getColor());
//  
//  Blynk.virtualWrite(V0, ledcolor.r, ledcolor.g, ledcolor.b);
//}

BLYNK_READ(V1)
{
  Blynk.virtualWrite(V1, brightness);
}

BLYNK_WRITE(V1)
{
  brightness = param.asInt();
  FastLED.setBrightness(brightness);
}

BLYNK_READ(V2)
{
  Blynk.virtualWrite(V2, brightnessDay);
}

BLYNK_WRITE(V2)
{
  brightnessDay = param.asInt();
  updateBrightness(true);
}

BLYNK_READ(V3)
{
  Blynk.virtualWrite(V3, brightnessNight);
}

BLYNK_WRITE(V3)
{
  brightnessNight = param.asInt();
  updateBrightness(true);
}

BLYNK_CONNECTED() {
  static bool isFirstConnect = true;
  if (isFirstConnect) {
    // Request Blynk server to re-send latest values
    Blynk.syncAll();
    isFirstConnect = false;
  }
  updateBrightness(true);
  Blynk.virtualWrite(V1, brightness);
}

//------------------------------------------------------------------------------
// Open Pixel Protocol callbacks:

// Callback when a full OPC Message has been received
void cbOpcMessage(uint8_t channel, uint8_t command, uint16_t length, uint8_t* data) {
  if(0 == channel || 1 == channel)
  {
    switch(command)
    {
      case 0x00:
                  for(uint8_t i = 0; i < length/3; i++)
                  {
                    leds[i].r = data[i*3+0];
                    leds[i].g = data[i*3+1];
                    leds[i].b = data[i*3+2];
                  }
                  FastLED.show(brightness);
                  break;
      case 0xFF: break;
    }
  }
}

// Callback when a client is connected
void cbOpcClientConnected(WiFiClient& client) {
  Serial.print("::cbOpcClientConnected(): New OPC Client: ");
#if defined(ESP8266) || defined(PARTICLE)
  Serial.println(client.remoteIP());
#else
  Serial.println("(IP Unknown)");
#endif
  displayClock = false;
}

// Callback when a client is disconnected
void cbOpcClientDisconnected(OpcClient& opcClient) {
  Serial.print("::cbOpcClientDisconnected(): Client Disconnected: ");
  Serial.println(opcClient.ipAddress);
  displayClock = true;
  clock.update(true);
  ani.transform(leds, leds_target, NUM_LEDS, true);
}

//------------------------------------------------------------------------------
//Ticker callback:
void cbTick()
{
  if(0 == leds[0].b) leds[0] = CRGB::Blue;
  else leds[0] = CRGB::Black;

  FastLED.show();
}

//------------------------------------------------------------------------------
//WiFiManager callback:
bool shouldSaveConfig = false;

void cbSaveConfig()
{
  Serial.println("::cbSaveConfig(): Should save config");
  shouldSaveConfig = true;
}

void cbConfigMode(WiFiManager *myWiFiManager)
{
  ticker.attach(0.2, cbTick);
}

//==============================================================================
// OpcServer Init
//==============================================================================
WiFiServer server = WiFiServer(OPC_PORT);
OpcClient opcClients[OPC_MAX_CLIENTS];
uint8_t buffer[OPC_BUFFER_SIZE * OPC_MAX_CLIENTS];
OpcServer opcServer = OpcServer(server, OPC_CHANNEL, opcClients, OPC_MAX_CLIENTS, buffer, OPC_BUFFER_SIZE);
//------------------------------------------------------------------------------




void setup () 
{
  Serial.begin(57600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  //FastLED.setMaxPowerInVoltsAndMilliamps(5,1000); 
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  //FastLED.setCorrection(0xF0FFF7);
  //FastLED.setTemperature(Tungsten40W);

  FastLED.setBrightness(brightness);

  fill_solid( &(leds[0]), NUM_LEDS, CRGB::Black);
  FastLED.show();

  ticker.attach(0.6, cbTick);

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(ntp_server, json["ntp_server"]);
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read


  WiFiManager wifiManager;
  wifiManager.setAPCallback(cbConfigMode);
  wifiManager.setSaveConfigCallback(cbSaveConfig);

  WiFiManagerParameter custom_ntp_server("server", "NTP server", ntp_server, 50);
  wifiManager.addParameter(&custom_ntp_server);

  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 33);
  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  if (!wifiManager.autoConnect("ESPClockAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(ntp_server, custom_ntp_server.getValue());
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["ntp_server"] = ntp_server;
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  Serial.print("Blynk token: ");
  Serial.println(blynk_token);

  IPAddress timeServerIP;
  WiFi.hostByName(ntp_server, timeServerIP);
  Serial.print("NTP Server: ");
  Serial.print(ntp_server);
  Serial.print(" - ");
  Serial.println(timeServerIP);

  Rtc.setup();
  Ntp.setup(timeServerIP);
  Rtc.setSyncProvider(&Ntp);
  setSyncProvider(getDateTimeFromRTC);

  updateBrightness();
  
  clock.setup(&(leds_target[0]), NUM_LEDS);
  clock.setTimezone(&CE);

  opcServer.setMsgReceivedCallback(cbOpcMessage);
  opcServer.setClientConnectedCallback(cbOpcClientConnected);
  opcServer.setClientDisconnectedCallback(cbOpcClientDisconnected);
  
  opcServer.begin();
  
  Serial.print("\nOPC Server: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(OPC_PORT);

  Blynk.config(blynk_token);
  ticker.detach();
}

void loop () 
{
  if (timeSet != timeStatus())
  {
    Serial.println("Time is not set. The time & date are unknown.");
    //TODO what shall we do in this case? 
  }

  updateBrightness();

  if(displayClock)
  {
    bool changed = clock.update();
    ani.transform(leds, leds_target, NUM_LEDS, changed);

    FastLED.show();
  }

  opcServer.process();

  Blynk.run();
}



