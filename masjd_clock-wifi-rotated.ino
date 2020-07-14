//for sqlite db
#include <stdio.h>
#include <stdlib.h>
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
#include "salat_art.h"

char GeoDate[10],Fajr[6],Shurooq[6], Dhuhr[6], Asr[6], Maghrib[6],Isha[6];
int minutes,ntp_time_as_int ;

//const char* ssid = "........";
//const char* password = "........";

const char* ntpServer = "0.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

 //store date str ex: GeoDate = 20200714
struct tm timeinfo;

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
#define FORMAT_SPIFFS_IF_FAILED false

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
  while(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(2000);
    //to do reboot if takes lore than 60s
  }
//struct tm {
//  int tm_sec;           /* Seconds. [0-60]      */
//  int tm_min;           /* Minutes. [0-59]      */
//  int tm_hour;          /* Hours.   [0-23]      */
//  int tm_mday;          /* Day.     [1-31]      */
//  int tm_mon;           /* Month.   [0-11]      */
//  int tm_year;          /* Year - 1900.         */
//  int tm_wday;          /* Day of week. [0-6]   */
//  int tm_yday;          /* Days in year.[0-365] */
//  int tm_isdst;         /* DST.     [-1/0/1]    */

ntp_time_as_int=(timeinfo.tm_year+1900)*10000+(timeinfo.tm_mon+1)*100+timeinfo.tm_mday ;
Serial.println(ntp_time_as_int);
  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

//read sqliteDB
sqlite3 *db1;
int rc;
sqlite3_stmt *res;
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
  
//  // for test only     
//   rc = db_exec(db1, "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = 1 AND GeoDate = 20200101");
//   if (rc != SQLITE_OK) {
//       sqlite3_close(db1);
//       return;
//   }
//  //end test

//char ntp_time_as_str[10];
//sprintf(ntp_time_as_str, "%d", ntp_time_as_int);
  String sql = "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = ";
  sql += "1";
  sql += " AND GeoDate = ";
  //sql += "20200714";
  //sql +=ntp_time_as_str ;
  sql +=ntp_time_as_int ;
  Serial.println(sql);
  rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    String resp = "Failed to fetch data: ";
    resp += sqlite3_errmsg(db1);
    Serial.println(resp.c_str());
    return;
  }

   Serial.println("salat times for today:");
  String resp = "";
  while (sqlite3_step(res) == SQLITE_ROW) {
    minutes = sqlite3_column_int(res, 1) ;
    snprintf(Fajr,6, "%02d:%02d", minutes / 60, minutes % 60);
    minutes = sqlite3_column_int(res, 2) ;
    snprintf(Shurooq,6, "%02d:%02d", minutes / 60, minutes % 60);
    minutes = sqlite3_column_int(res, 3) ;
    snprintf(Dhuhr,6, "%02d:%02d", minutes / 60, minutes % 60);
    minutes = sqlite3_column_int(res, 4) ;
    snprintf(Asr,6, "%02d:%02d", minutes / 60, minutes % 60);
    minutes = sqlite3_column_int(res, 5) ;
    snprintf(Maghrib,6, "%02d:%02d", minutes / 60, minutes % 60);
    minutes = sqlite3_column_int(res, 6) ;
    snprintf(Isha,6, "%02d:%02d", minutes / 60, minutes % 60);    
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
 //salat.draw(vga, (millis() / 50) % 1, vga.xres/4*1, vga.yres/2); // no rotation
 salat_art.draw(vga, (millis() / 50) % 1, vga.xres/2, vga.yres/4*1); // draw at Right edge.
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


gfx.setFont(&FreeMonoBoldOblique24pt7b);
gfx.setTextColor(0xFFFF);
int space=48;
int top=115;
gfx.setCursor(0, top);
gfx.print(Fajr);
gfx.setCursor(0, top+space);
gfx.print(Shurooq);
gfx.setCursor(0, top+space*2);
gfx.print(Dhuhr);
gfx.setCursor(0, top+space*3);
gfx.print(Asr);
gfx.setCursor(0, top+space*4);
gfx.print(Maghrib);
gfx.setCursor(0, top+space*5);
gfx.print(Isha);
//end write salat times


//draw rectangle to write clock time above it
//gfx.fillRect(0, 40, 300, 35, 0x5f11);
//gfx.fillRect(0, 40, 300, 35, 0x0000);

//int x=0;
//int y=40;
//int w=300;
//int h=35;
//int c=2;
//vga.fillRect(x + 50, y + 15, 35 - c / 8, 3 + c / 16, vga.RGB(255 - c, c, 0));

//gfx.setCursor(35, 70);
//gfx.setTextColor(63488, 0xFFFF);
//gfx.print(&timeinfo, "%H:%M:%S");
}


void printLocalTimeVGA()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
gfx.fillRect(35, 40,300-70, 35, 0xffff); //white
gfx.setTextColor(0);
gfx.setCursor(35, 70);
gfx.print(&timeinfo, "%H:%M:%S");
//Serial.println(&timeinfo, "%H:%M:%S");
//or convert to string and print to serial:
//char s[9];
//strftime(s,sizeof(s),"%Y%m%d", &timeinfo);
//String asString(s);
//Serial.println(s); 
}

void loop()
{

  //gfx.fillScreen(0);
  //vga.clear(vga.RGB(0x000000));

  printLocalTimeVGA();
  delay(1000);
  

}
