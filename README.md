# Word clock

This project provides the code for a word clock. The layout is based on the famous original. Here color is controlled via webpage.  
There is a layout for a small and a big word clock. They differ by 4 LEDs for the minutes.

## Hardware:
+ ESP8266 NodeMCU
+ LED stripe APA102C, alternativly SK9822
+ Realtime clock DS3231

## Software Libraries:
+ FastLED library
+ WifiManager
+ Timezone
+ Timelib
+ ArduinoJSON
+ SPIFFS support

Please check the code for the exact version and source url of each library.  
Running the clock requires at least 1A current. Please make sure that your power supply is stable enough.

## Notes:
Each letter is enlighted from a single LED. It's recommended to use a light gate for each single LED (not only words). 

### update 2019-11-17  
added dialekt selection: dropdown box with "fränkisch", "baierisch" and "hochdeutsch".

### update 2020-09-20
switched to Visual Studio Code and PlatformIO

