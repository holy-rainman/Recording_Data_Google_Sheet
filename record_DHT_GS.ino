#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
#include <GS_SDHelper.h>

#include "DHTesp.h"
DHTesp dht;

#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

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

void setup()
{ dht.setup(D7, DHTesp::DHT11);

  Serial.begin(9600);
  Serial.println();
  Serial.println();

  configTime(0, 0, ntpServer);
  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

  // Connect to Wi-Fi
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) 
  { Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Set the callback for Google API access token generation status (for debug only)
  GSheet.setTokenCallback(tokenStatusCallback);

  // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
  GSheet.setPrerefreshSeconds(10 * 60);

  // Begin the access token generation for Google API authentication
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop()
{ bool ready = GSheet.ready();

  if (ready && millis() - lastTime > timerDelay)
  { lastTime = millis();
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
    // valueRange.set("values/[3]/[0]", pres);

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
}

void tokenStatusCallback(TokenInfo info)
{ if (info.status == token_status_error)
  { GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  }
  else
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
}
