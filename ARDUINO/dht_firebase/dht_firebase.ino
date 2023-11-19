#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#define DHTPIN 4  //D2
#define DHTTYPE DHT11
#define trigPin D5  //D5 14
#define echoPin D6  //D6 12
#define soilPin A0
#define tankRelay 15        //D8
#define irrigationRelay 13  //D7

float duration, distance;
float moisture;
float tankDepth = 20.0;  //In centimetres
float waterLevel;

float upperMoisture = 70.0;
float lowerMoisture = 50.0;
float upperWaterLevel = 60.0;
float lowerWaterLevel = 20.0;

DHT dht(DHTPIN, DHTTYPE);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "CHELIZ"
#define WIFI_PASSWORD "Seiko555"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBtyicIbpKhu_iU6jB-wnNuhvwYc42oBoA"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://dht11-2f7c5-default-rtdb.firebaseio.com/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

//unsigned long sendDataPrevMillis = 0;
//int count = 0;
bool signupOK = false;

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionally you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 10800, 60000);

void setup() {
  pinMode(DHTPIN, INPUT);
  dht.begin();
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);          // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);           // Sets the echoPin as an Input
  pinMode(irrigationRelay, OUTPUT);  //Sets the relay as an Output
  pinMode(tankRelay, OUTPUT);        //Sets the relay as an Output
  pinMode(soilPin, INPUT);           //Sets the soilPin as an Input
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  timeClient.begin();

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.get(&fbdo, "/upperMoisture")) {

      if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_integer) {
        upperMoisture = fbdo.to<int>();
        Serial.println(fbdo.to<int>());
      }

    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.get(&fbdo, "/lowerMoisture")) {

      if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_integer) {
        lowerMoisture = fbdo.to<int>();
        Serial.println(fbdo.to<int>());
      }

    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.get(&fbdo, "/upperWaterLevel")) {

      if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_integer) {
        upperWaterLevel = fbdo.to<int>();
        Serial.println(fbdo.to<int>());
      }

    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.get(&fbdo, "/lowerWaterLevel")) {

      if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_integer) {
        lowerWaterLevel = fbdo.to<int>();
        Serial.println(fbdo.to<int>());
      }

    } else {
      Serial.println(fbdo.errorReason());
    }

  } else {
    Serial.println("GET failed");
  }
  Serial.println(upperMoisture);
  Serial.println(lowerMoisture);
  Serial.println(upperWaterLevel);
  Serial.println(lowerWaterLevel);
  delay(1000);

  //Get water level
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) * 0.0344;

  waterLevel = tankDepth - distance;
  Serial.println(distance);
  Serial.println(waterLevel);
  if ((waterLevel >= 0.0) & (waterLevel <= tankDepth)) {
    if (waterLevel <= ((lowerWaterLevel / 100.0) * tankDepth)) {
      //ON
      digitalWrite(tankRelay, HIGH);
    } else if (waterLevel > ((lowerWaterLevel / 100.0) * tankDepth)) {
      //check pump state
      if (digitalRead(tankRelay) == 0) {
        //pump = 0
        digitalWrite(tankRelay, LOW);
      } else if (digitalRead(tankRelay) == 1) {
        if (waterLevel <= ((upperWaterLevel / 100.0) * tankDepth)) {
          //pump = on
          digitalWrite(tankRelay, HIGH);
        } else if (waterLevel > ((upperWaterLevel / 100.0) * tankDepth)) {
          //pump = off
          digitalWrite(tankRelay, LOW);
        }
      }
    }
  } else {
    //pump = 0
    digitalWrite(tankRelay, LOW);
  }

  // Get soil moisture
  moisture = analogRead(soilPin);  //Read analog moisture
  Serial.println(moisture);
  moisture = constrain(moisture, 330, 560);    //Keep the ranges!
  moisture = map(moisture, 330, 560, 100, 0);  //Map moisture : 330 will be 100% (wet soil) and 560 will be 0% (dry soil)

  if (moisture <= lowerMoisture) {
    //ON
    digitalWrite(irrigationRelay, HIGH);
  } else if (moisture > lowerMoisture) {
    //check pump state
    if (digitalRead(irrigationRelay) == 0) {
      //pump = 0
      digitalWrite(irrigationRelay, LOW);
    } else if (digitalRead(irrigationRelay) == 1) {
      if (moisture <= upperMoisture) {
        //pump = on
        digitalWrite(irrigationRelay, HIGH);
      } else if (moisture > upperMoisture) {
        //pump = off
        digitalWrite(irrigationRelay, LOW);
      }
    }
  }


  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  float h = dht.readHumidity();
  //Serial.println(h);

  float t = dht.readTemperature();
  //Serial.println(t);

  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setFloat(&fbdo, "humidity/" + timeClient.getFormattedTime(), h)) {
      //      Serial.println("PASSED");
      Serial.print("Humidity: ");
      Serial.println(h);

    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "temperature/" + timeClient.getFormattedTime(), t)) {
      //      Serial.println("PASSED");
      Serial.print("Temperature: ");
      Serial.println(t);
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    float percentFull = (waterLevel / tankDepth) * 100.0;
    if (Firebase.RTDB.setFloat(&fbdo, "percentFull/" + timeClient.getFormattedTime(), percentFull)) {
      //Serial.println("PASSED");
      Serial.print("Water Level: ");
      Serial.println(waterLevel);
      Serial.print("Percent Full: ");
      Serial.println(percentFull);
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "moisture/" + timeClient.getFormattedTime(), moisture)) {
      //Serial.println("PASSED");
      Serial.print("Moisture: ");
      Serial.println(moisture);
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "irrigationRelay/" + timeClient.getFormattedTime(), digitalRead(irrigationRelay))) {
      //      Serial.println("PASSED");
      Serial.print("Irrigation Relay: ");
      Serial.println(digitalRead(irrigationRelay));
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "tankRelay/" + timeClient.getFormattedTime(), digitalRead(tankRelay))) {
      //Serial.println("PASSED");
      Serial.print("Tank Relay: ");
      Serial.println(digitalRead(tankRelay));
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

  Serial.println("______________________________");
}