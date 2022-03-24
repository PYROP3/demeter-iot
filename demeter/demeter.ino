#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "secrets.h"

#ifdef DEBUG
#define LOOP_DELAY 30 * 1000
#else
#define LOOP_DELAY 10 * 60 * 1000
#endif

#define READ_DELAY 100

#define PIN_HYGRO A0
#define PIN_RELAY 2
#define PIN_BUOY 0

#define PUMP_ON_THRESHOLD 500
#define PUMP_OFF_THRESHOLD 270

#define PUMP_ON LOW
#define PUMP_OFF HIGH

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(PIN_HYGRO, INPUT);
  pinMode(PIN_BUOY, INPUT);
  pinMode(PIN_RELAY, OUTPUT);

  digitalWrite(PIN_RELAY, PUMP_OFF);
}

void handleHumidity(int humidity) {
#ifdef DEBUG
  Serial.printf("Handling humidity: %d vs %d\n", humidity, PUMP_ON_THRESHOLD);
#endif
  if (humidity > PUMP_ON_THRESHOLD) {
    Serial.printf("Handling humidity: %d vs %d\n", humidity, PUMP_ON_THRESHOLD);
    Serial.println("Activating water pump");
    digitalWrite(PIN_RELAY, PUMP_ON);
    do {
      delay(LOOP_DELAY);
    } while (analogRead(PIN_HYGRO) > PUMP_OFF_THRESHOLD);
    digitalWrite(PIN_RELAY, PUMP_OFF);
    Serial.println("Deactivated water pump");
#ifdef DEBUG
  } else {
    Serial.println("No action required");
#endif
  }
}

void loop() {  
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

#ifdef DEBUG
    Serial.print("[HTTP] begin...\n");
#endif

    // configure traged server and url
    http.begin(client, "http://" SERVER_IP "/log"); //HTTP
    http.addHeader("Content-Type", "application/json");

#ifdef DEBUG
    Serial.print("[HTTP] POST...\n");
#endif

    int raw_humidity = analogRead(PIN_HYGRO);
#ifdef DEBUG
    Serial.printf("Raw humidity: %d\n", raw_humidity);
#endif
    handleHumidity(raw_humidity);
    
    int humidity = map(raw_humidity, 0, 1023, 0, 100);
    Serial.printf("Mapped humidity: %d\n", humidity);
    
    int buoyStatus = digitalRead(PIN_BUOY);

    // start connection and send HTTP header and body
    int httpCode = http.POST(String("") + "{\"hygro\":" + humidity + ",\"buoy\":" + buoyStatus + "}");

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
#ifdef DEBUG
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
#endif
    } else {
      Serial.printf("[HTTP] POST... failed, error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
  }

  delay(LOOP_DELAY);
}
