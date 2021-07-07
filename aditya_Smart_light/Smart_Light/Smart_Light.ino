#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

#define DBG_OUTPUT_PORT Serial
#define TRIGGER_PIN 0


const int RedLED = 16; // D0
const int BlueLED = 4;    // D2
const int GreenLED = 5; // D1
const char* mdnsName = "SmartLight";
const char HOSTNAME[] = "SmartLight";
char* password = "password";
char* ssid = "Smart Light";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

#include "FSWebServer.h"
#include "request_handlers.h"
#include "definitions.h"

void setup() {
  // put your setup code here, to run once:

  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  
  pinMode(TRIGGER_PIN, INPUT);
  
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");

  MDNS.begin(HOSTNAME);
  DBG_OUTPUT_PORT.print("mDNS responder started: http://");
  DBG_OUTPUT_PORT.print(HOSTNAME);
  DBG_OUTPUT_PORT.println(".local");

  // Setup: SPIFFS Webserver handler
  server.on("/", HTTP_GET, []() {
    if (!handleFileRead("/index.html")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  //server.on("/upload", handleMinimalUpload);
  server.on("/upload", HTTP_GET, []() {
    if (!handleFileRead("/upload.html")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

 
    

}



bool rainbow = false;             // The rainbow effect is turned off on startup

unsigned long prevMillis = millis();
int hue = 0;

void loop() {


if ( digitalRead(TRIGGER_PIN) == LOW ) {
      Serial.println("online");
    WiFiManager wifiManager;
    if (!wifiManager.startConfigPortal("Smart Light")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  else {

  if ( digitalRead(TRIGGER_PIN) == HIGH ) {
   Serial.println("offline");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  webSocket.begin();                          // start the websocket server
  
     }
}



  
  // put your main code here, to run repeatedly:
  server.handleClient();
  MDNS.update();
  webSocket.loop();
  if (rainbow) {                              // if the rainbow effect is turned on
    if (millis() > prevMillis + 32) {
      if (++hue == 360)                       // Cycle through the color wheel (increment by one degree every 32 ms)
        hue = 0;
      setHue(hue);                            // Set the RGB LED to the right color
      prevMillis = millis();
    }
  }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);

      if (payload[0] == '#') {            // we get RGB data
        uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
        int r = ((rgb >> 20) & 0x3FF);                     // 10 bits per color, so R: bits 20-29
        int g = ((rgb >> 10) & 0x3FF);                     // G: bits 10-19
        int b =          rgb & 0x3FF;                      // B: bits  0-9

        analogWrite(RedLED, r);
        analogWrite(GreenLED, g);
        analogWrite(BlueLED, b);
      } else if (payload[0] == 'R') {                      // the browser sends an R when the rainbow effect is enabled
        rainbow = true;
      } else if (payload[0] == 'N') {                      // the browser sends an N when the rainbow effect is disabled
        rainbow = false;
      }
      break;
  }
}
