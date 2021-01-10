#include <Arduino.h>

#include <FS.h>                  // From the WifiManager AutoConnectWithFSParameters: this needs to be first, or it all crashes and burns...

#define FASTLED_ESP8266_RAW_PIN_ORDER  

#include <TimeLib.h>              // http://www.arduino.cc/playground/Code/Time Time, by Michael Margolis, includes  https://github.com/PaulStoffregen/Time V1.5
#include "CRTC.h"
#include "CNTPClient.h"
#include <Timezone.h>             // https://github.com/JChristensen/Timezone v1.2.2
#include <FastLED.h>              // https://github.com/FastLED/FastLED v3.2.10
#include "CFadeAnimation.h"
#include "CClockDisplay.h"

#include <ESP8266WiFi.h>          // ESP8266 board package
#include <ESP8266mDNS.h>          // ESP8266 board package

#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager v0.14.0
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson v6.11.5

#define WEBSERVER_H               // https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer v1.2.3

#include <Ticker.h>               // ESP8266 board package
#include <LittleFS.h>             // ESP8266 board package
#include <ArduinoOTA.h>


Ticker ticker;
AsyncWebServer server(80);

/*
 * ------------------------------------------------------------------------------
 * LED configuration
 * ------------------------------------------------------------------------------
 */

#define DATA_PIN 13
#define CLOCK_PIN 14

CRGB leds[118];
CRGB leds_target[118];
bool leds_fill[118];

uint8_t gHue = 0; // demo mode hue

bool small_clock = true;
uint8_t num_leds = 118;

CRGB color_correction(160, 230, 255);
ESPIChipsets chipset = APA102;

/*
 * ------------------------------------------------------------------------------
 * Configuration parameters configured by the WiFiManager and stored in the FS
 * ------------------------------------------------------------------------------
 */

//The default values for the NTP server
char ntp_server[50] = "0.de.pool.ntp.org";
char clock_version[15] = __DATE__;

/*
* ------------------------------------------------------------------------------
* Wifi/NTP status
* ------------------------------------------------------------------------------
*/
enum eWifiState
{
	e_disconnected = 0,
	e_connected = 1,
	e_reconnect_needed = 2,
};

eWifiState eWiFiConnState = eWifiState::e_disconnected;
WiFiEventHandler disconnectedEventHandler;
WiFiEventHandler connectedEventHandler;

/*
 * ------------------------------------------------------------------------------
 * Clock configuration/variables
 * ------------------------------------------------------------------------------
 */

uint8_t brightnessNight =  5;
uint8_t brightnessDay   = 50;

TimeElements DayTime = {0, 0, 6, 0, 0, 0, 0};
TimeElements NightTime = {0, 0, 22, 0, 0, 0, 0};

bool displayClock = true;  // as opposite to demo mode


CRTC Rtc;
CNTPClient Ntp;

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

CFadeAnimation ani;
CClockDisplay clockDisp;


bool updateBrightness()
{
	time_t local( CE.toLocal(now()) );

	if ((hour(local) * 60 + minute(local)) >= (NightTime.Hour * 60 + NightTime.Minute) 
	||  (hour(local) * 60 + minute(local)) < (DayTime.Hour * 60 + DayTime.Minute))
	{
		if (FastLED.getBrightness() != brightnessNight)
		{
			Serial.println("Update Brightness: NIGHT");
			FastLED.setBrightness(brightnessNight);
			return true;
		}
	}
	else
	{
		if (FastLED.getBrightness() != brightnessDay)
		{
			Serial.println("Update Brightness: DAY");
			FastLED.setBrightness(brightnessDay);
			return true;
		}
	}
	return false;
}

TimeElements fromTimeString(String time)
{
	TimeElements tm;
	tm.Hour = time.substring(0, time.indexOf(":") ).toInt();
	tm.Minute = time.substring(time.indexOf(":") + 1).toInt();
	return tm;
}

String getTimeString(TimeElements time)
{
	String sTime;
	char cBuff[6];
	sprintf(cBuff, "%02d:%02d", time.Hour, time.Minute);

	sTime = cBuff;
	return sTime;
}

String getTimeString(time_t time)
{
	char cBuff[9];
	sprintf(cBuff, "%02d:%02d:%02d", hour(time), minute(time), second(time));
	return cBuff;
}

String getHexColor(const CRGB& color)
{
	char cBuff[8];
	sprintf(cBuff, "#%02x%02x%02x", color.r, color.g, color.b);
	return cBuff;
}

CRGB HtmlHexColor(const String& color)
{
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;

	if(color.length() == 7)
	{
		sscanf(color.c_str(), "#%02x%02x%02x", &r, &g, &b);
	}
	
	return CRGB(r, g , b);
}

String toString(ESPIChipsets chipset)
{
	switch(chipset)
	{
		case LPD6803: return F("LPD6803"); break;
		case LPD8806: return F("LPD8806"); break;
		case WS2801: return F("WS2801"); break;
		case WS2803: return F("WS2803"); break;
		case SM16716: return F("SM16716"); break;
		case P9813: return F("P9813"); break;
		case APA102: return F("APA102"); break;
		case SK9822: return F("SK9822"); break;
		case DOTSTAR: return F("DOTSTAR"); break;
	}
	return F("UNKNOWN");
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
	if (eWiFiConnState == eWifiState::e_disconnected)
	{
		Serial.println("WiFi Adapter disconnected from WLAN");
	}
	else if (eWiFiConnState == eWifiState::e_reconnect_needed)
	{
		Serial.println("WiFi Adapter reconnect needed");
	}
	else if (eWiFiConnState == eWifiState::e_connected)
	{
		Serial.println("WiFi Adapter connected to WLAN");
	}
	
	return Rtc.now();
}


//------------------------------------------------------------------------------
//Ticker callback:
void cbTick()
{
	if (0 == leds[0].b)
	{
		leds[0] = CRGB::Blue;
		leds[1] = CRGB::Black;
	}
	else
	{
		leds[0] = CRGB::Black;
		leds[1] = CRGB::Blue;
	}

	FastLED.show();
}

//------------------------------------------------------------------------------
//WiFiManager callback:
void cbConfigMode(WiFiManager *myWiFiManager)
{
	ticker.attach(0.2, cbTick);
}

/*
* ------------------------------------------------------------------------------
* clock settings
* brightness day/night + color Red/Green/Blue
* NTPserver
* ------------------------------------------------------------------------------
*/

void serializeConfig(JsonDocument& doc)
{
	doc["display"]["color"] = getHexColor(clockDisp.getColor());
	doc["display"]["correction"] = getHexColor(color_correction);
	doc["display"]["mode"] = clockDisp.getColorMode();
	doc["display"]["dialect"] = clockDisp.getDialekt();

	doc["brightness"]["daystart"] = getTimeString(DayTime);
	doc["brightness"]["dayend"] = getTimeString(NightTime);
	doc["brightness"]["day"] = brightnessDay;
	doc["brightness"]["night"] = brightnessNight;

	doc["ntp"]["server"] = ntp_server;

	doc["clock"]["chipset"] = chipset;
	doc["clock"]["small"] = small_clock;
}

void writeSettings()
{
	Serial.println("writing config");

	StaticJsonDocument<1024> jsonBuffer;

	serializeConfig(jsonBuffer);

	File configFile = LittleFS.open("/config.json", "w");
	if ( !configFile )
	{
		Serial.println("failed to open config file for writing");
		return;
	}

	serializeJson(jsonBuffer, configFile);

	configFile.close();

	Serial.println("done saving");
}

void deserializeConfig(JsonDocument& doc)
{
	clockDisp.setColor(HtmlHexColor(doc["display"]["color"]));
	color_correction = HtmlHexColor(doc["display"]["correction"]);
	clockDisp.setColorMode((CClockDisplay::eColorMode) doc["display"]["mode"]);
	clockDisp.setDialekt((CClockDisplay::eDialekt) doc["display"]["dialect"]);

	DayTime = fromTimeString(doc["brightness"]["daystart"]);
	NightTime = fromTimeString(doc["brightness"]["dayend"]);
	brightnessDay = doc["brightness"]["day"];
	brightnessNight = doc["brightness"]["night"];

	strcpy(ntp_server, doc["ntp"]["server"]);

	chipset = doc["clock"]["chipset"];
	small_clock = doc["clock"]["small"];
}

void printConfig()
{
	Serial.println("Configuration:");
	Serial.printf("  Color:            %s\n", getHexColor(clockDisp.getColor()).c_str());
	Serial.printf("  Correction:       %s\n", getHexColor(color_correction).c_str());
	Serial.printf("  Mode:             %d\n", clockDisp.getColorMode());
	Serial.printf("  Dialect:          %d\n", clockDisp.getDialekt());
	Serial.printf("  Day start:        %s\n", getTimeString(DayTime).c_str());
	Serial.printf("  Day end:          %s\n", getTimeString(NightTime).c_str());
	Serial.printf("  Brightness Day:   %d\n", brightnessDay);
	Serial.printf("  Brightness Night: %d\n", brightnessNight);
	Serial.printf("  NTP server:       %s\n", ntp_server);
	Serial.printf("  LED Chipset:      %s\n", toString(chipset).c_str());
	Serial.printf("  Clock size:       %s\n", small_clock ? "small" : "big");
	return;
}

void readSettings()
{
	if ( LittleFS.begin() )
	{
		if ( LittleFS.exists("/config.json") )
		{
			Serial.println("INIT: reading config file");

			File configFile = LittleFS.open("/config.json", "r");
			if (configFile)
			{
				Serial.println("INIT: opened config file");

				size_t size = configFile.size();
				if (size > 1024)
				{
					Serial.println("INIT: config file size is too large");
					return;
				}

				// Allocate a buffer to store contents of the file.
				std::unique_ptr<char[]> buf(new char[size]);

				configFile.readBytes(buf.get(), size);

				StaticJsonDocument<1024> json;
				auto error = deserializeJson(json, buf.get());
				if (error)
				{
					Serial.println("INIT: failed to parse config file");
					return;
				}

				configFile.close();

				serializeJsonPretty(json, Serial);
				Serial.println();

				deserializeConfig(json);

				printConfig();
			}
			else
			{
				// no config file found, use defaults
				Serial.println("INIT: config not found, using defaults");
			}
		}
		else
		{
		Serial.println("no config file.");
		}
	}
	else
	{
		Serial.println("Failed to mount file system");
	}

}

void SetNewNtp(const char* ntp)
{
	strcpy(ntp_server, ntp);

	IPAddress timeServerIP;
	WiFi.hostByName(ntp_server, timeServerIP);
	Serial.printf("NTP Server: %s - %s\n", ntp_server, timeServerIP.toString().c_str());
	Ntp.setup(timeServerIP);
	return;
}


void WiFiReconnect()
{
	if (eWifiState::e_reconnect_needed == eWiFiConnState)
	{
		Serial.println("WiFi reconnected. Trying to establish all connections again ... ");
		SetNewNtp(ntp_server);
		eWiFiConnState = eWifiState::e_connected;
	}
}

// WiFi disconnect handler
// WiFiEventHandler onStationModeDisconnected(std::function<void(const WiFiEventStationModeDisconnected&)>);
void onWiFiDisconnected(const WiFiEventStationModeDisconnected& event)
{
	if (eWiFiConnState != eWifiState::e_disconnected)
	{
		Serial.println("event - WiFi Adapter disconnected from WLAN");
	}
	eWiFiConnState = eWifiState::e_disconnected;
}

// WiFi connect handler
// WiFiEventHandler onStationModeConnected(std::function<void(const WiFiEventStationModeConnected&)>);
void onWiFiConnected(const WiFiEventStationModeConnected& event)
{
	if (eWiFiConnState != eWifiState::e_reconnect_needed)
	{
		Serial.println("event - WiFi Adapter connected to WLAN");
	}
	eWiFiConnState = eWifiState::e_reconnect_needed;
}

String processor(const String& var)
{
	if(var == F("BRIGHTNESS_DAY"))
	    return String(brightnessDay);
	if(var == F("BRIGHTNESS_NIGHT"))
	    return String(brightnessNight);
	if(var == F("DAY_TIME"))
	    return getTimeString(DayTime);
	if(var == F("NIGHT_TIME"))
	    return getTimeString(NightTime);
	if(var == F("LEDCOLOR_R"))
	    return String(clockDisp.getColor().r);
	if(var == F("LEDCOLOR_G"))
	    return String(clockDisp.getColor().g);
	if(var == F("LEDCOLOR_B"))
	    return String(clockDisp.getColor().b);
	if(var == F("LEDCOLOR"))
	    return getHexColor(clockDisp.getColor());
	if(var == F("CORRECTION"))
	    return getHexColor(color_correction);
	if(var == F("LED_STRIPE"))
	{
		return toString(chipset);
	}
	if(var == F("NTP_SERVER"))
	    return ntp_server;
	if(var == F("NTP_LAST_SYNC"))
	{
		time_t lastSync (CE.toLocal(Ntp.getLastSync()));
		return getTimeString(lastSync);
	}
	if(var == F("ESP_FREE_SKETCH_SPACE"))
	    return String(ESP.getFreeSketchSpace());
	if(var == F("ESP_SKETCH_SIZE"))
	    return String(ESP.getSketchSize());
	if(var == F("CLOCK_VERSION"))
		return clock_version;
	if(var == F("DIALECT_BAYER_SELECTED"))
	    return (clockDisp.getDialekt() == CClockDisplay::e_Bayerisch) ? " selected" : "";
	if(var == F("DIALECT_FRANK_SELECTED"))
	    return (clockDisp.getDialekt() == CClockDisplay::e_Frankisch) ? " selected" : "";
	if(var == F("DIALECT_HOCH_SELECTED"))
	    return (clockDisp.getDialekt() == CClockDisplay::e_Hochdeutsch) ? " selected" : "";
	if(var == F("COLOR_MODE_SOLID"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeSolid) ? " selected" : "";
	if(var == F("COLOR_MODE_GRADIENT"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeGradient) ? " selected" : "";
	if(var == F("COLOR_MODE_GLITTER"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeGlitter) ? " selected" : "";
	if(var == F("COLOR_MODE_RAINBOW1"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeRainbow_1) ? " selected" : "";
	if(var == F("COLOR_MODE_RAINBOW2"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeRainbow_2) ? " selected" : "";
	if(var == F("COLOR_MODE_RAINBOW3"))
		return (clockDisp.getColorMode() == CClockDisplay::e_ModeRainbow_3) ? " selected" : "";
	if(var == F("SSID"))
		return (WiFi.SSID());
	if(var == F("CLOCK_SIZE"))
	    return (small_clock ? F("SMALL") : F("BIG"));
	return var;
}

void sendJsonResponse(AsyncWebServerRequest* request)
{
	AsyncResponseStream * response = request->beginResponseStream("application/json");
	StaticJsonDocument<1024> jsonBuffer;
	serializeConfig(jsonBuffer);
	serializeJson(jsonBuffer, *response);
	request->send(response);
	return;
}

/*
* ------------------------------------------------------------------------------
* setup MCU
* ------------------------------------------------------------------------------
*/
void setup()
{
	Serial.begin(115200);

	Serial.print("compiled: ");
	Serial.print(__DATE__);
	Serial.println(__TIME__);

	readSettings();

	// init LEDs
	if(small_clock)
	{
		num_leds = 118;
	} else {
		num_leds = 114;
	}

	Serial.print("init LED stripe ");
	Serial.println(toString(chipset));
	switch(chipset)
	{
		case APA102:
			FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, num_leds);
			break;
		case SK9822:
			FastLED.addLeds<SK9822, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(12)>(leds, num_leds);
			break;
	}

    clockDisp.setup( &(leds_target[0]), &(leds_fill[0]), num_leds);
	clockDisp.isSmallClock(small_clock);
	clockDisp.setTimezone(&CE);

	FastLED.setCorrection(color_correction);
	fill_solid( &(leds[0]), num_leds, CRGB::Black);
	FastLED.show();

	ticker.attach(0.6, cbTick);

	WiFiManager wifiManager;
	wifiManager.setAPCallback(cbConfigMode);

	if (!wifiManager.autoConnect("WordClock")) 
	{
		Serial.println("failed to connect and hit timeout");
		delay(3000);

		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}

	//if you get here you have connected to the WiFi
	Serial.println("connected...yeey :)");
	WiFi.setAutoReconnect(true);

	// first connection establish, lets initialize the handlers needed for proper reconnect
	Serial.println("connecting WIFI event handler");
	disconnectedEventHandler = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
	connectedEventHandler    = WiFi.onStationModeConnected(&onWiFiConnected);

	Serial.print("IP number assigned by DHCP is ");
	Serial.println(WiFi.localIP());

	Serial.print("SSID:");
	Serial.println(WiFi.SSID());

	Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

	Rtc.setup();
	SetNewNtp(ntp_server);
	Rtc.setSyncProvider(&Ntp);
	setSyncProvider(getDateTimeFromRTC);
 
	clockDisp.update(true);
	updateBrightness();
	FastLED.show();

	// ArduinoOTA setup
	ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
		LittleFS.end();
        Serial.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();
 
	// Start the Web server
	server.on("/ntp", [](AsyncWebServerRequest *request){
		if(request->hasParam("server", true))
		{
			Serial.println("POST Request");
		    AsyncWebParameter* p = request->getParam("server", true);
			strcpy(ntp_server, p->value().c_str());
		}
		if(request->hasParam("sync", true))
		{
			time_t tTime = Ntp.now();
			if (tTime == 0)
			{
				delay(1000);
				Ntp.now();
			}
		}
		sendJsonResponse(request);
	});
	server.on("/demo", [](AsyncWebServerRequest *request){
		displayClock = !displayClock;
		sendJsonResponse(request);
	});
	server.on("/dialect", [](AsyncWebServerRequest *request){
		if(request->hasParam("value", true))
		{
			AsyncWebParameter* p = request->getParam("value", true);
			clockDisp.setDialekt((CClockDisplay::eDialekt)p->value().toInt());
			clockDisp.update(true);
		}
		sendJsonResponse(request);
	});
	server.on("/brightness", [](AsyncWebServerRequest *request){
		if(request->hasParam("daystart", true))
		{
			AsyncWebParameter* p = request->getParam("daystart", true);
			DayTime = fromTimeString(p->value());
			updateBrightness();
			clockDisp.update(true);
		}
		if(request->hasParam("dayend", true))
		{
			AsyncWebParameter* p = request->getParam("dayend", true);
			NightTime = fromTimeString(p->value());
			updateBrightness();
			clockDisp.update(true);
		}
		if(request->hasParam("day", true))
		{
			AsyncWebParameter* p = request->getParam("day", true);
			brightnessDay = p->value().toInt();
			updateBrightness();
			clockDisp.update(true);
		}
		if(request->hasParam("night", true))
		{
			AsyncWebParameter* p = request->getParam("night", true);
			brightnessNight = p->value().toInt();
			updateBrightness();
			clockDisp.update(true);
		}
		sendJsonResponse(request);
	});
	server.on("/color", [](AsyncWebServerRequest *request){
		if(request->hasParam("hexcolor", true))
		{
			AsyncWebParameter* p = request->getParam("hexcolor", true);
			clockDisp.setColor(HtmlHexColor(p->value()));
			clockDisp.update(true);
		}
		if(request->hasParam("correction", true))
		{
			AsyncWebParameter* p = request->getParam("correction", true);
			color_correction = HtmlHexColor(p->value());
			FastLED.setCorrection(color_correction);
			clockDisp.update(true);
		}
		if(request->hasParam("mode", true))
		{
			AsyncWebParameter* p = request->getParam("mode", true);
			clockDisp.setColorMode((CClockDisplay::eColorMode) p->value().toInt());
			clockDisp.update(true);
		}
		sendJsonResponse(request);
	});
	server.on("/config", [](AsyncWebServerRequest *request){
		if(request->hasParam("save", true))
		{
			AsyncWebParameter* p = request->getParam("save", true);
			if(p->value() == "true")
				writeSettings();
		}
		request->send(LittleFS, "/config.json", "application/json");
	});
	server.on("/restart", [](AsyncWebServerRequest *request){
        request->send(200);
		ESP.restart();
	});
	server.on("/wifi", [](AsyncWebServerRequest *request){
		if(request->hasParam("reset", true))
		{
			AsyncWebParameter* p = request->getParam("reset", true);
			if(p->value() == "true")
			{
				request->send(200);
				WiFiManager wifiManager;
  				wifiManager.resetSettings();
				ESP.restart();
			}
		}
		request->send(200);
	});
	server.serveStatic("/", LittleFS, "/www/").setTemplateProcessor(processor);
	server.serveStatic("/config.json", LittleFS, "/config.json");
	server.onNotFound([](AsyncWebServerRequest *request){
		request->send(404);
	});
	server.begin();
	Serial.println("Webserver - server started");

	ticker.detach();
}

void demo()
{
	FastLED.setBrightness(brightnessDay);

	// CONFETTI: random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, num_leds, 10);
	int pos = random16(num_leds);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);

	FastLED.show();

	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / 120);

	// do some periodic updates
	EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
	return;
}

/*
* ------------------------------------------------------------------------------
* Loop - main routine
* ------------------------------------------------------------------------------
*/
void loop () 
{
	// reconnect WiFiManager, if needed
	WiFiReconnect();

	// Handle OTA
	ArduinoOTA.handle();

	if (timeSet != timeStatus())
	{
		Serial.println("Time is not set. The time & date are unknown.");
	}

	if (displayClock)
	{
		updateBrightness();

		bool bChanged = false;
		if (clockDisp.getColorMode() == CClockDisplay::e_ModeGlitter)
		{
			bChanged = clockDisp.update(true);
			ani.transform2(leds, leds_target, num_leds);
		}
		else
		{
			bChanged = clockDisp.update();
			ani.transform(leds, leds_target, num_leds, bChanged);
		}

		FastLED.show();
	}
	else
	{
		demo();
	}

	MDNS.update();
}
