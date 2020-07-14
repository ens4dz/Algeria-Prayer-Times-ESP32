//for sqlite db
//#include <stdio.h>
//#include <stdlib.h>
#include <sqlite3.h>
#include <FS.h>
#include "SPIFFS.h"
// This file should be compiled with 'Partition Scheme' (in Tools menu)
// set to 'NO OTA (2MB APP/2MB FATFS)' 

//fot vga
#include "Adafruit_GFX.h"
#include <WiFi.h>
#include "time.h"
#include <ESP32Lib.h>
#include <GfxWrapper.h>
#include <Fonts/FreeMonoBoldOblique24pt7b.h>

//backround image:
#include "salathalf.h"
//#include "salat.h"


//const char* ssid = "........";
//const char* password = "........";
const char* ssid = "AT-4GLTE-H";
const char* password = "wassim@gmail.com";

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
//VGA3Bit vga;
VGA3BitI vga;
GfxWrapper<VGA3BitI> gfx(vga, 400, 300);



//for sqlite

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

int db_open(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
}

//end sqlite

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

//read sqliteDB
   Serial.setDebugOutput(true);
sqlite3 *db1;
int rc;
sqlite3_stmt *res;
int rec_count = 0;
const char *tail;
   
   if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
       Serial.println("Failed to mount file system");
       return;
   }

   // list SPIFFS contents
   File root = SPIFFS.open("/");
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   if (!root.isDirectory()) {
       Serial.println(" - not a directory");
       return;
   }
   File file = root.openNextFile();
   while (file) {
       if (file.isDirectory()) {
           Serial.print("  DIR : ");
           Serial.println(file.name());
       } else {
           Serial.print("  FILE: ");
           Serial.print(file.name());
           Serial.print("\tSIZE: ");
           Serial.println(file.size());
       }
       file = root.openNextFile();
   }


   sqlite3_initialize();

   if (db_open("/spiffs/Algeria.db", &db1))
       return;
  // for test only     
   rc = db_exec(db1, "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = 1 AND GeoDate = 20190719");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }



  String sql = "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = ";
  sql += "1";
  sql += " AND GeoDate = ";
  sql += "20190719";
  //sql += "'";
  rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    String resp = "Failed to fetch data: ";
    resp += sqlite3_errmsg(db1);
    Serial.println(resp.c_str());
    return;
  }

  String resp = "";
  while (sqlite3_step(res) == SQLITE_ROW) {
    resp = "";
    resp += sqlite3_column_int(res, 1); //Fajr
    resp += " ";
    resp += sqlite3_column_int(res, 2); //Shurooq
    resp += " ";
    resp += sqlite3_column_int(res, 3); //Dhuhr
    resp += " ";
    resp += sqlite3_column_int(res, 4); //Asr
    resp += " ";
    resp += sqlite3_column_int(res, 5); //Maghrib
    resp += " ";
    resp += sqlite3_column_int(res, 6);//Isha
    Serial.println(resp);
  }
String Shurooq, Dhuhr, Asr, Maghrib,Isha ;
//Fajr=""
//Fajr=sqlite3_column_int(res, 0)%60

   static char Fajr[8];
    int minute = sqlite3_column_int(res, 1) ;
    Serial.println(minute);
    char * p = Fajr;
    p += sprintf(p, "Fajr=%02d:%02d", minute / 60, minute % 60);
    *p++ = 0;
    Serial.println(Fajr);


sqlite3_finalize(res);
  sqlite3_close(db1);
//end sqlitedb read








  
  vga.init(vga.MODE400x300, redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  //using adafruit gfx
  //Note that the text background color is not supported for custom fonts. For these,
  //you will need to determine the text extents and explicitly draw a filled rectangle before drawing the text.
  //gfx.setTextSize(4);
  //setting the text color to white with opaque black background:
  //vga.setTextColor(vga.RGB(0xffffff), vga.RGBA(0, 0, 0, 255));
  //rotatae -90° :
  gfx.setRotation(3);

  //draw background image:
 //salat.draw(vga, (millis() / 50) % 1, vga.xres/2, vga.yres/2);
 salat.draw(vga, (millis() / 50) % 1, vga.xres/2, vga.yres/4*1);
 vga.show();

//every day
//draw salat times only if the day changed, 
//draw rectangle to write salat times
gfx.fillRect(0, 90, 145, 300, 0x0);

// x=0;
// y=90;
// w=145;
// h=300;
// c=0;
//vga.print("fillRect(x, y, w, h, c)");

gfx.setCursor(0, 120);
gfx.setFont(&FreeMonoBoldOblique24pt7b);
gfx.setTextColor(0xFFFF);
//to do : get salat time from db
//gfx.print(&timeinfo, "%H:%M\n%H:%M\n%H:%M\n%H:%M\n%H:%M\n%H:%M");
//end write salat times
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
//int x=0;
//int y=40;
//int w=300;
//int h=35;
//int c=2;
//vga.fillRect(x + 50, y + 15, 35 - c / 8, 3 + c / 16, vga.RGB(255 - c, c, 0));

gfx.setCursor(35, 70);
gfx.setTextColor(63488, 0xFFFF);
gfx.print(&timeinfo, "%H:%M:%S");
//static char up[16];
//  int sec = millis() / 1000;
//  int min = sec / 60;
//  int hr = min / 60;
//  int dy = hr / 24;
//  char * p = up;
//  p+=sprintf(p, "%H:%M:%S",&timeinfo);
//  *p++ = 0;
//vga.print(up);


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
