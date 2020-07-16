//working ntp, fixed without ntp lib, improve time at boot
//to do: improve http server (index html is inspired from ESP32-CAM example. )
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5 
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
// vga bluePin = 32; 
// vga  greenPin = 33; 
// vga  redPin = 25;
// vga  hsyncPin = 26;
// vga  vsyncPin = 27;

//libs for sqlite db
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

//clock rtc "Rtc_by_Makuna"
#include <ThreeWire.h>  
#include <RtcDS1302.h> 

//Html server:
#include <WiFi.h>
#include <Hash.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "index.h" //html page
#include <EEPROM.h> // city,config

//VGA Device
const int bluePin = 32; //d32
const int greenPin = 33; 
const int redPin = 25;
const int hsyncPin = 26;
const int vsyncPin = 27;
VGA3Bit vga; //free mem wifi off:91796 - 
GfxWrapper<VGA3Bit> gfx(vga, 400, 300); //screen resolution
//VGA3BitI vga;
//GfxWrapper<VGA3BitI> gfx(vga, 400, 300); //screen resolution


//for ntp time 
struct tm timeinfo; 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3601; //+1GMT
const int   daylightOffset_sec = 0; 

//var to save sql request
char HijriDate[13],GeoDate[13],Fajr[6],Shurooq[6], Dhuhr[6], Asr[6], Maghrib[6],Isha[6];
int d,H_year, H_month,H_day ;// hidjri date
int minutes ;

// store config in EEPROM.
//see this:https://roboticsbackend.com/arduino-store-int-into-eeprom/
//warning int size is 4 bytes in esp32, https://github.com/espressif/arduino-esp32/issues/1745
short Auto_fan,city_id,hijri_shift ; //size of int is 2bytes

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}
int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

AsyncWebServer server(80);
bool force2updateFromNtp= false ;
bool alarmTest=false ;
String CurrentTime;

RtcDateTime ntp_time,now;
ThreeWire myWire(4,5,2); // IO, SCLK, CE - RTC clock
RtcDS1302<ThreeWire> Rtc(myWire);

const char* ssid = "AT-4GLTE-H";
const char* password = "wassim@gmail.com";

//const char* ssid = "m";
//const char* password = "12345678+";
//// Set your Static IP address
//IPAddress local_IP(192, 168, 43, 77);
//// Set your Gateway IP address
//IPAddress gateway(192, 168, 43, 1);
//IPAddress subnet(255, 255, 255, 0);
//IPAddress primaryDNS(8, 8, 8, 8);   //optional
//IPAddress secondaryDNS(1, 1, 1, 1); //optional

#define RELAY 19

//for sqlite functions
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
//end sqlite functions


void ntpSyn(AsyncWebServerRequest *request){
    force2updateFromNtp=true;
    AsyncWebServerResponse * response = request->beginResponse(200, "text/plain", "syncronizing time..");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void Status(AsyncWebServerRequest *request){
    static char json_response[1024];
    char * p = json_response;
    *p++ = '{';
    p+=sprintf(p, "\"Auto_fan\":%d,", Auto_fan);
    p+=sprintf(p, "\"city\":%2d,", city_id);
    p+=sprintf(p, "\"hijri_shift\":%d", hijri_shift);

    *p++ = '}';
    *p++ = 0;
    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void  getCameraIndex(AsyncWebServerRequest *request){
AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
response->addHeader("Content-Encoding", "gzip");
request->send(response);
}


void uptimer(AsyncWebServerRequest *request){
  static char json_response[128];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  int dy = hr / 24;

    char * p = json_response;
    *p++ = '{';
    //p+=sprintf(p, "\"uptime\":\"%02d يوم و %02d-%02d-%02d----02d:02d:02d\"", dy,hr % 24, min % 60,sec %60,now.Hour(),now.Minute(),now.Second());
    p+=sprintf(p, "\"uptime\":\"");
    p+=sprintf(p," مشتغل منذ ");
    p+=sprintf(p,"%02d ", dy);
    p+=sprintf(p,"يوم و ");
    p+=sprintf(p,"%02d-%02d-%02d ",sec %60, min % 60,hr % 24);
    p+=sprintf(p,"الساعة حاليا: ");
    p+=sprintf(p,"%02d:%02d:%02d",now.Hour(),now.Minute(),now.Second());
    p+=sprintf(p," التاريخ الهجري: ");
    p+=sprintf(p,"%s", HijriDate);
    p+=sprintf(p," التاريخ الميلادي: ");
    p+=sprintf(p,"%s", GeoDate);    
    p+=sprintf(p,"\""); //end of json
    *p++ = '}';
    *p++ = 0;              
    AsyncWebServerResponse * response = request->beginResponse(200, "application/json", json_response);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void setCameraVar(AsyncWebServerRequest *request){
    if(!request->hasArg("var") || !request->hasArg("val")){
        request->send(404);
        return;
    }
    String var = request->arg("var");
    const char * variable = var.c_str();
    int val = atoi(request->arg("val").c_str());
    int res = 0;
  
    if(!strcmp(variable, "Auto_fan")){
       Auto_fan=val;
       writeIntIntoEEPROM(0, val);
        if (EEPROM.commit()) {
          Serial.println("EEPROM Auto_fan successfully committed");
        } else {
          Serial.println("ERROR! Auto_fan EEPROM commit failed");
        }
    }
     else if(!strcmp(variable, "city")) {
       city_id=val;
       writeIntIntoEEPROM(2, val);
        if (EEPROM.commit()) {
          Serial.println("EEPROM city successfully committed");
        } else {
          Serial.println("ERROR! city EEPROM commit failed");
        }
    }
      
      else if(!strcmp(variable, "hijri_shift")) {
       hijri_shift=val;
       writeIntIntoEEPROM(4, val);
        if (EEPROM.commit()) {
          Serial.println("EEPROM hijri_shift successfully committed");
        } else {
          Serial.println("ERROR! hijri_shift EEPROM commit failed");
        }
    }

     else {
        request->send(404);
        return;
    }

    AsyncWebServerResponse * response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

///zzz

#define countof(a) (sizeof(a) / sizeof(a[0]))
String printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return datestring;
}


String printTime(const RtcDateTime& dt)
{
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return datestring;
}

//int printTint(const RtcDateTime& dt)
//{
//      return dt.Hour()*100+dt.Minute();
//}
//int printTintSec(const RtcDateTime& dt)
//{
//      return dt.Second();
//}
//int printDay(const RtcDateTime& dt)
//{
//    return dt.DayOfWeek();
//}

void updateFromNTP(){
  
while ( WiFi.status() != WL_CONNECTED ) {
    Serial.println("connecting wifi");
    delay ( 500 );
    Serial.print ( "connecting to WiFi." );
  }

  while ( !getLocalTime(&timeinfo) ){
  Serial.println("updating time..");
  Serial.println("updating time..");
  delay ( 1000 );}
  
  Serial.print("Use this URL to connect: ");
  Serial.print("https://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  //ntp_time.InitWithEpoch32Time(timeClient.getEpochTime()+1);
  time_t timeSinceEpoch = mktime(&timeinfo);
  ntp_time.InitWithEpoch32Time(timeSinceEpoch+gmtOffset_sec);
  Rtc.SetDateTime(ntp_time);
}

void printLocalTimeVGA()
{
gfx.fillRect(35, 40,300-70, 35, 0xffff); //white
gfx.setTextColor(0);
gfx.setCursor(35, 70);
gfx.print(printTime(now));
//Serial.println(&now, "%H:%M:%S");
//or convert to string and print to serial:
//char s[9];
//strftime(s,sizeof(s),"%Y%m%d", &now);
//String asString(s);
//Serial.println(s); 
}

void Adan()
{
Serial.print("Adan time: ");
//Todo other cmd..
        if (Auto_fan) {
             digitalWrite(RELAY,HIGH) ;
             Serial.println("Fans turned on!");
        } 
}


void setup(){
  Serial.begin(115200);
  EEPROM.begin(512);
  Auto_fan = readIntFromEEPROM(0);
  city_id = readIntFromEEPROM(2);
  hijri_shift = readIntFromEEPROM(4);
  Serial.print("EEPROM.read(4) is: ");
  Serial.println(EEPROM.read(4));
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY,LOW);

  
//     if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//    Serial.println("WiFi Failed to configure");}

    WiFi.begin(ssid, password);
    delay(3000);  
    Serial.print(WiFi.localIP());
    configTime(gmtOffset_sec, daylightOffset_sec,  ntpServer);    
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);

    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing
        Serial.println("RTC lost confidence in the DateTime!");
        updateFromNTP();
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

  if ( getLocalTime(&timeinfo) ){        //get time from ntp test
    time_t timeSinceEpoch = mktime(&timeinfo);
    ntp_time.InitWithEpoch32Time(timeSinceEpoch+gmtOffset_sec);
  }

    now = Rtc.GetDateTime();
    Serial.print("time from NTP:" ); 
    Serial.println(printDateTime(ntp_time)); //print ntp time
    Serial.print("time from RTC:" );     
    Serial.println(printDateTime(now)); //print rtc time
    Serial.println(now); 
    if (now < compiled) 
    {
        Serial.println("RTC is older than ntp_time time!  (Updating DateTime)");
        updateFromNTP();
    }

  Serial.print("FreeMem before http:") ;   //FreeMem before http:255468
  Serial.println(ESP.getFreeHeap() ) ; 

/////  http server
    server.on("/ntp", HTTP_GET, ntpSyn);
    server.on("/control", HTTP_GET, setCameraVar);
    server.on("/status", HTTP_GET, Status);
    server.on("/uptime", HTTP_GET, uptimer);
   server.on("/alarm", HTTP_GET, [](AsyncWebServerRequest *request){  alarmTest=true ; });
    server.on("/", HTTP_GET, getCameraIndex);
   server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){  ESP.restart();  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
//  WiFi.disconnect(true);
//  WiFi.mode(WIFI_OFF);
//disconnect WiFi as it's no longer needed
  
////// end server


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

  Serial.print("FreeMem before sqlite:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
//  FreeMem before sqlite:231304

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
//sprintf(ntp_time_as_str, "%d", rtc_time_as_int);


//rtc_time_as_int=(now.tm_year+1900)*10000+(now.tm_mon+1)*100+now.tm_mday ;
    char dateForSQL[11];
    snprintf_P(dateForSQL,countof(dateForSQL),
            PSTR("%04u%02u%02u"),now.Year(),now.Month(),now.Day());
            //String asString(dateForSQL)
    snprintf_P(GeoDate,countof(GeoDate),
            PSTR("%04u-%02u-%02u"),now.Year(),now.Month(),now.Day());  // only for print in uptime func
            
  String sql = "SELECT GeoDate,Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha FROM mawakit_midnight WHERE MADINA_ID = ";
  sql += city_id;
  sql += " AND GeoDate = ";
  //sql += "20200714";
  //sql +=ntp_time_as_str ;
  sql +=dateForSQL ;
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

  Serial.print("FreeMem before get hijri date:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
  //FreeMem before get hijri date:188824

  //get hijri date
  resp="";
  sql = "SELECT HijriDate FROM itc_tab_hijri_geo_date WHERE GeoDate = ";
  sql +=dateForSQL ;
  Serial.println(sql);
  rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
  if (rc != SQLITE_OK) {
    resp = "Failed to fetch data of hijri time: ";
    resp += sqlite3_errmsg(db1);
    Serial.println(resp.c_str());
    return;
  }


  Serial.println("hijri_shift: ");
  Serial.println(hijri_shift);
  Serial.println("today HijriDate: ");
  while (sqlite3_step(res) == SQLITE_ROW) {
    d=sqlite3_column_int(res, 0) ; //14391120
    H_year=d/10000;               // 1439
    d=d%10000  ;              //1120
    H_month=d/100;           //11
    H_day=d%100+hijri_shift ;           //20
      if(H_day==31) {H_day=1 ; H_month=H_month+1 ;}
        else if (H_day==0) {H_day=30 ; H_month=H_month-1 ;}
      
    snprintf(HijriDate,11, "%04d-%02d-%02d", H_year, H_month,H_day);  
    resp = sqlite3_column_int(res, 0); //Fajr
    Serial.println(resp);
  }
  sqlite3_finalize(res);
  
  sqlite3_close(db1);
  //end sqlitedb read

  Serial.print("FreeMem before vga init:") ;  
  Serial.println(ESP.getFreeHeap() ) ; 
  //FreeMem before vga init:188824
  
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
gfx.fillRect(0, 90, 145, 300, 0x0); //big rec
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


}

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 1000;  //the value is a number of milliseconds

void loop() {
    startMillis = millis();
    printLocalTimeVGA();
    now = Rtc.GetDateTime();
    if (!now.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
        ESP.restart() ;
    }
    
  //if (Auto_fan) {Serial.println("---No Alarm !---") ; } else {Serial.println("SedikiNawri Club");}
 // CurrentTime=printTime(now);
  //sql format to do use that better than chars
  //int TimeNoSec=now.Hour()*60+now.Minute() ; 
      char TimeNoSec[6];
      snprintf_P(TimeNoSec, 
          countof(TimeNoSec),
          PSTR("%02u:%02u"),
          now.Hour(),
          now.Minute());
          
//Serial.print("TimeNoSec:  ");
//Serial.println(TimeNoSec);
//Serial.print("Fajr:  ");
//Serial.println(Fajr);                
    if(now.Second()==0){
      Serial.print("now.Second()==0");

        if (now.DayOfWeek() == 6 ) {
          // todo: Djomoaa addons
        }
               
        //test if salat time Fajr,Shurooq, Dhuhr, Asr, Maghrib,Isha;      
        if(!strcmp(Fajr, TimeNoSec)) Adan();
        else if(!strcmp(Shurooq, TimeNoSec)) Adan();
        else if(!strcmp(Dhuhr,TimeNoSec)) Adan();
        else if(!strcmp(Asr, TimeNoSec)) Adan();
        else if(!strcmp(Maghrib, TimeNoSec)) Adan();
        else if(!strcmp(Isha, TimeNoSec)) Adan();
           
    } //if now.second 
    

  if (force2updateFromNtp) updateFromNTP() ;
  
  AsyncElegantOTA.loop();
  
  if (alarmTest) {
    Serial.println("Alarm!");
      digitalWrite(RELAY,HIGH) ;
      delay(5000);
      digitalWrite(RELAY,LOW);
      alarmTest=false;
      }
  
  Serial.print("FreeMem:") ;  
  Serial.println(ESP.getFreeHeap() ) ;  
  Serial.print("time for 1 loop:") ;  
  Serial.println(millis() - startMillis) ; //8
  Serial.print("HijriDate: ");
  Serial.println(HijriDate);
  delay(1000-8);
}
