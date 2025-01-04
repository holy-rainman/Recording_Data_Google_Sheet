#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "REPLACE_WITH_YOUR_ID"
#define BLYNK_TEMPLATE_NAME "REPLACE_WITH_YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "REPLACE_WITH_YOUR_AUTH_TOKEN"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <Arduino.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
#include <GS_SDHelper.h>

#include "DHTesp.h"
DHTesp dht;

char ssid[] = "REPLACE_WITH_YOUR_SSID";
char pass[] = "REPLACE_WITH_YOUR_PASSWORD";

BlynkTimer timer;

BLYNK_WRITE(V0)
{ int pb = param.asInt();
  beep(1,50);
  digitalWrite(D0,pb); 
}
BLYNK_WRITE(V1)
{ int pb = param.asInt();
  beep(1,50);
  digitalWrite(D1,pb);  delay(500);
  digitalWrite(D2,pb);  delay(500);
}

int status=0;
void kelip()
{ digitalWrite(D3,status);
  status =! status;
}
void getPB()
{ int pbD5 = digitalRead(D5);
  if(pbD5==1) Blynk.virtualWrite(V2,"ON");
  if(pbD5==0) Blynk.virtualWrite(V2,"OFF");
}
void beep(int bil, int tempoh)
{ for(int i=0;i<bil;i++)
  { tone(D6,400); delay(tempoh);
    noTone(D6);   delay(tempoh);
  }
}
char dhtData[100];
void getDHT11()
{ float humidity    = dht.getHumidity();
  float temperature = dht.getTemperature();

  sprintf(dhtData,"Humid: %.1f%\tTemp:%.1f",humidity,temperature);
  Serial.println(dhtData);
  Blynk.virtualWrite(V3,humidity);
  Blynk.virtualWrite(V4,temperature);
}

//============================================== Google Sheet Code Start
// Google Project ID
#define PROJECT_ID "REPLACE_WITH_YOUR_PROJECT_ID"

// Service Account's client email
#define CLIENT_EMAIL "REPLACE_WITH_YOUR_CLIENT_EMAIL"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\ REPLACE_WITH_YOUR_PRIVATE_KEY\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "YOUR_SPREADSHEET_ID";

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 5000;

void tokenStatusCallback(TokenInfo info);
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; 

unsigned long getTime() 
{ time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) 
    return(0);
  time(&now);
  return now;
}
void tokenStatusCallback(TokenInfo info)
{ if (info.status == token_status_error)
  { GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  }
  else
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
}
void recordGoogleSheet()
{ bool ready = GSheet.ready();

  FirebaseJson response;

  Serial.println("\nAppend spreadsheet values...");
  Serial.println("----------------------------");

  FirebaseJson valueRange;
  // Get DHT11 data
  float humidity    = dht.getHumidity();
  float temperature = dht.getTemperature();
  // Get timestamp
  epochTime = getTime();

  valueRange.add("majorDimension", "COLUMNS");
  valueRange.set("values/[0]/[0]", epochTime);
  valueRange.set("values/[1]/[0]", humidity);
  valueRange.set("values/[2]/[0]", temperature);

  bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
  if (success)
  { response.toString(Serial, true);
    valueRange.clear();
  }
  else
    Serial.println(GSheet.errorReason());
  
  Serial.println();
  Serial.println(ESP.getFreeHeap());
}
//============================================== Google Sheet Code End

void setup()
{ pinMode(D0,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D5,INPUT);
  pinMode(D6,OUTPUT);
  
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  beep(3,50);

  timer.setInterval(500,kelip);
  timer.setInterval(10,getPB);        

  dht.setup(D7, DHTesp::DHT11);
  timer.setInterval(2000,getDHT11);
  timer.setInterval(5000,recordGoogleSheet);

  //============================================ Google Sheet Setup
  configTime(0, 0, ntpServer);
  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);
  GSheet.setTokenCallback(tokenStatusCallback);
  GSheet.setPrerefreshSeconds(10 * 60);
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop()
{ Blynk.run();
  timer.run();
}

