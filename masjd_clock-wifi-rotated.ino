#include "Adafruit_GFX.h"
#include <WiFi.h>
#include "time.h"
#include <ESP32Lib.h>
#include <GfxWrapper.h>
#include <Fonts/FreeMonoBoldOblique24pt7b.h>
#include <Fonts/FreeMonoBoldOblique18pt7b.h>

//backround image:
#include "salat.h"

const char* ssid = "........";
const char* password = "........";

const char* ntpServer = "0.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//VGA Device
const int bluePin = 12;
const int greenPin = 13;
const int redPin = 15;
const int hsyncPin = 14;
const int vsyncPin = 2;
//VGA3BitI vga;
VGA3BitI vga;
GfxWrapper<VGA3BitI> gfx(vga, 400, 300);

void setup()
{
  Serial.begin(115200);

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.println(WiFi.localIP());

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  vga.init(vga.MODE400x300, redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  //using adafruit gfx
  //Note that the text background color is not supported for custom fonts. For these,
  //you will need to determine the text extents and explicitly draw a filled rectangle before drawing the text.
  gfx.setFont(&FreeMonoBoldOblique18pt7b);
  //gfx.setTextSize(4);
  gfx.setTextColor(63488, 0xFFFF);
  //setting the text color to white with opaque black background:
  //vga.setTextColor(vga.RGB(0xffffff), vga.RGBA(0, 0, 0, 255));
  //rotatae -90° :
  gfx.setRotation(3);

  //draw background image:
  salat.draw(vga, (millis() / 50) % 1, vga.xres/2, vga.yres/2);
  vga.show();
  
}

void printLocalTimeVGA()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

//draw rectangle to write clock time above it
gfx.fillRect(0, 40, 300, 35, 0x5f11);
gfx.setCursor(35, 70);
gfx.setTextColor(63488, 0xFFFF);
gfx.print(&timeinfo, "%H:%M:%S");

//to do : draw salat times only if the day changed, 
//draw rectangle to write salat times
gfx.fillRect(0, 90, 145, 300, 0x0);
gfx.setCursor(0, 120);
gfx.setFont(&FreeMonoBoldOblique24pt7b);
gfx.setTextColor(0xFFFF);
//to do : get salat time from db
gfx.print(&timeinfo, "%H:%M\n%H:%M\n%H:%M\n%H:%M\n%H:%M\n%H:%M");
//end write salat times
}


void loop()
{
  Serial.println(ESP.getFreeHeap() ) ;
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_DEFAULT) ) ;

  //gfx.fillScreen(0);
  //vga.clear(vga.RGB(0x000000));

  printLocalTime(); //serial
  printLocalTimeVGA();
  delay(1000);
  

}
