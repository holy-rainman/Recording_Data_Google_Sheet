#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
#include <GS_SDHelper.h>

#include "DHTesp.h"
DHTesp dht;

#define WIFI_SSID "JHChua@unifi"
#define WIFI_PASSWORD "adamdanial123"

// Google Project ID
#define PROJECT_ID "uthm-aim"

// Service Account's client email
#define CLIENT_EMAIL "uthm-aim@uthm-aim.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCljneEKazXa70e\nDsAtjhrJ15iX6JHoyxLWSKsn/SwcMayJxII6ooTqE7nNu3D7MN3lIdFGYfXpsE4M\nWSvxRULiF1HzyDFPQ98a87aoUEEU8y9Kyj5BUTCKzA4EEyDOJg3VeDjfoyAhGS6f\n0tWqx3AvyEpub164XdTCv4Jc65GNzd/f8HAkd/86C2nbtO2maU9z4wPyNgOkNFnZ\nw2tsmhJJRY5W1RpEFL9+M4+400bej6wxCyogTJrwI1lDSIeAAOlVaD4VWZm1JkG4\nKE+KCuLZSOigCxfMUhjc6OlP/sSGgAlSD4iKnER6WKwJPLhqkj5+TKt5vdb6hkN5\nK7yd8oP3AgMBAAECggEANxi/MLY35edepGqvTVneE29TQrbtHCa9KaA8ic3pSKbF\nb4H8/yuIBWi0LMIlOwuLxU4oWf6O+wsoJlbk12I//INbTEDDHEiPruYN010GE8VM\n51yANHejKoX3YibFg6XJc9ITR2X/8tPxHtJpNnEyJkzHbJJW2Q+tF3TpmyfwpFqm\n2j1YCzUau8ZApRJ0t2S8H9YqXSPbQ2dwiQgM6Wik6sVDvd8twCYvTq4lHGsv25PS\n8Q5oKzw2nvTSgt0vT3rlFiDGnxOltcZX5kNN1p25MLiCp3ONUT+u8SIseQx4Vaut\n4XYQkDejQEYskgWqLHMPVpAyxY3JhyeXTo3hfeyXoQKBgQDZnUUqOpyKn69oB9E/\nM3doD1s2L/GsbJV5smKp4kdfatBW2pLizXU7Ik52FjMmqHcQhqiv2wztAYIlrsbR\nJcAk6VDajXK8UXwWhgfBySZ9IRmf6GUCDC5a9aR2eAsj+z4y97oRbR+42eIbbMUg\nzqbZNjlNUR7bt/GzBnPhmskIVwKBgQDCwnDXNgOZKqXywMMSRaUO+HjTCwHfX+s3\nf56E7DN8JmCHgQNlc+mau2698oHXBOz9zc3j8PUf7ietkj6zeZVwyWHiMQwauvAi\nO6ZUjsjCBIdYijjsYln0dhZmula11wc5xf8Txxcs14lFNpnN/MY7P4xHAmo1eQRt\nuhvvOf+dYQKBgASonEFyLcW982hbunwK4wSER6SaVCcnz+9iBUCzymldhQZG+ZwH\nX/obGujmQsQSGTJuX5AcNWOhg6LA4hq6HynrIML9AjbL28czqVjZsIw/OCg3Nmpb\nzmY+Gf47RanIiCVZLk+PMX2olHGX24R/SA0gXwurr+huQKtfeP2cba9TAoGAKWP5\nZ0NrcFnH+tdMnc1/TrcvtmpYc4iHDLxqjAVGy9O8IBAjEKSZuPCskGHBhljdpPLg\nFtlWvvp2JEU4Xhwwzk9+JfwWfnfMIPpcVwwhTlp4CTEW9+XPCE0wZZSkaPDR8E1L\n8IAXRMhpu/PmrAhUeLMAtPama8bUUh2DYkwJLwECgYEAiy1V0cYgQgA3m+9YeUT3\nv4FG8fhyER01UqPLt/3xlnhliQnkI8N4betzdh5Z0GnWkf0MXvBU07zd3FbENugY\nKWR/LKdp6wXiHfstvqzjFRyoghezbV073yQ68tpluF51jixpjBk8lBO0FpawZqo7\nxPJylaN2av4PDwaO7VJHwc4=\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1BKjyRPRSwTbcmuXfgUbJCzPYMgqjOJcYQeaZmL8xXqU";

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
