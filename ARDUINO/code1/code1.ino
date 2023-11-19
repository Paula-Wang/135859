#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_dhtX0.h>

Adafruit_dhtX0 dht;


#define trigpin 11
#define echopin 10
#define soilpin A0
#define relay 8

float temp;
float humidity;
float moisture;
float duration, distance;
String payload[5];



SoftwareSerial A9G(2, 3);  // RX, TX


//PDP CONTEXT
const char *CID = "1";
const char *PDP_TYPE = "IP";
const char *APN = "safaricom.co.ke";
char PDP_CONTEXT[64];

MQTT CONNECTION
const char *HOST = "io.adafruit.com";
const char *PORT = "1883";
const char *CLIENT_ID = "Pauline1";  //REMEMBER TO CHANGE THIS (DEVICE NAME AS INDICATED ON THE CASING)
const char *ALIVE_SECONDS = "120";   //2 minutes
const char *CLEAN_SESSION = "0";
const char *QOS = "1";
const char *DUP_FLAG = "0";
const char *REMAIN_FLAG = "0";  // Changed the retain flag to 0/Flase
const char *USERNAME = "Emphatic_frog2";
const char *PASSWORD = "aio_hYHf12CebgDmKdvDCEg43Lljo8Hw";
char CONNECT[64];

//MQTT CONNECTION
// const char *HOST = "156.0.232.201";
// const char *PORT = "1883";
// const char *CLIENT_ID = "SU030J2023";  //REMEMBER TO CHANGE THIS (DEVICE NAME AS INDICATED ON THE CASING)
// const char *ALIVE_SECONDS = "120";     //2 minutes
// const char *CLEAN_SESSION = "0";
// const char *QOS = "1";
// const char *DUP_FLAG = "0";
// const char *REMAIN_FLAG = "0";  // Changed the retain flag to 0/Flase
// const char *USERNAME = "iotlab";
// const char *PASSWORD = "80d18b0d";
// char CONNECT[64];

//MQTT PUBLISH
const char *PUB_TOPIC1 = "Emphatic_frog2/feeds/irrigation.temperature";
const char *PUB_TOPIC2 = "Emphatic_frog2/feeds/irrigation.humidity";
// const char *PUB_TOPIC3 = "Emphatic_frog2/feeds/irrigation.water-level";
// const char *PUB_TOPIC4 = "Emphatic_frog2/feeds/irrigation.soil-moisture";
// const char *PUB_TOPIC5 = "Emphatic_frog2/feeds/irrigation.pump-state";

char PAYLOAD[64];
char PUBLISH[128];

String _buffer;
unsigned int interval = 1740;  // seconds
unsigned long int last_millis;
unsigned long int begin_millis;
unsigned long int end_millis;


void (*resetFunc)(void) = 0;  // declare reset fuction at address 0

void setup() {

  Serial.begin(115200);
  A9G.begin(115200);
  A9G.setTimeout(2000);
  _buffer.reserve(64);
  dht.begin();
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  pinMode(soilpin, INPUT);
  pinMode(relay, OUTPUT);

  //Turn off brown-out enable in software
  //MCUCR = bit(BODS) | bit(BODSE);
  //MCUCR = bit(BODS);


  delay(10000);
  //RESET
  A9G.println(F("AT+RST=1"));
  A9G.flush();
  Serial.println(F("AT+RST=1"));
  delay(2500);

  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      _buffer = A9G.readString();
      if (_buffer.indexOf("READY") >= 0) {
        Serial.println("READY");
        break;
      } else {
        delay(1000);
      }
    }
  } while (1);

  // AT-OK TEST
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      A9G.println(F("AT"));
      A9G.flush();  //Wait for string to  be sent
      Serial.println(F("AT"));
      delay(100);
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        delay(1000);
      }
    }
  } while (1);


  // ATTACH
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      break;
    } else {
      A9G.println(F("AT+CGATT=1"));
      A9G.flush();  //Wait for string to  be sent
      Serial.println(F("AT+CGATT=1"));
      delay(100);
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        delay(2000);
      }
    }
  } while (1);

  // DEFINE PDP CONTEXT
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      sprintf(PDP_CONTEXT,
              "AT+CGDCONT=%s,\"%s\",\"%s\"",
              CID, PDP_TYPE, APN);
      A9G.println(PDP_CONTEXT);
      A9G.flush();  //Wait for string to  be sent
      Serial.println(PDP_CONTEXT);
      delay(100);
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        delay(1000);
      }
    }
  } while (1);


  // ACTIVATE PDP CONTEXT
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      A9G.println(F("AT+CGACT=1,1"));
      A9G.flush();  //Wait for string to  be sent
      Serial.println(F("AT+CGACT=1,1"));
      delay(100);
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        delay(2000);
      }
    }
  } while (1);
}


void loop() {
  //Connect to MQTT broker
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      sprintf(CONNECT,
              "AT+MQTTCONN=\"%s\",%s,\"%s\",%s,%s,\"%s\",\"%s\"",
              HOST, PORT, CLIENT_ID, ALIVE_SECONDS, CLEAN_SESSION, USERNAME, PASSWORD);
      A9G.println(CONNECT);
      A9G.flush();  //Wait for string to  be sent
      delay(100);
      Serial.println(CONNECT);
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        delay(5000);
        A9G.println(F("AT+MQTTCONN?"));
        A9G.flush();  //Wait for string to  be sent
        Serial.println(F("AT+MQTTCONN?"));
        delay(100);
        _buffer = A9G.readString();
        if (_buffer.indexOf("1") >= 0) {
          Serial.println("1");
          break;
        } else {
          delay(2000);
          Serial.println("Retrying...");
        }
      }
    }
  } while (1);

  //Send Telemetry
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {


      begin_millis = millis();
      //Read Sensors' Values

      sensors_event_t event;
      dht.temperature().getEvent(&event);
      temp = event.temperature;

      // Get humidity event and print its moisture.
      dht.humidity().getEvent(&event);
      humidity = event.relative_humidity;

      // Get soil moisture
      moisture = analogRead(soilpin);               //Read analog moisture
      moisture = constrain(moisture, 400, 1023);    //Keep the ranges!
      moisture = map(moisture, 400, 1023, 100, 0);  //Map moisture : 400 will be 100 and 1023 will be 0


      //Get water level
      digitalWrite(trigpin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigpin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigpin, LOW);

      duration = pulseIn(echopin, HIGH);
      distance = (duration / 2) * 0.0344;
      bool pumpstate = digitalRead(relay);

      payload[0] = String(temp);
      payload[1] = String(humidity);
      //payload[2] = String(moisture);
      //payload[3] = String(distance);
      //payload[4] = String(pumpstate);

      Serial.println(temp);
      Serial.println(humidity);
      //Serial.println(moisture);
      //Serial.println(distance);
      //Serial.println(pumpstate);
      Serial.println("");

      if (moisture <= 30) {
        //ON
        digitalWrite(relay, HIGH);
      } else if (moisture > 30) {
        //check pump state
        if (digitalRead(relay) == 0) {
          //pump = 0
          digitalWrite(relay, LOW);
        } else if (digitalRead(relay) == 1) {
          if (moisture <= 60) {
            //pump = on
            digitalWrite(relay, HIGH);
          } else if (moisture > 60) {
            //pump = off
            digitalWrite(relay, LOW);
          }
        }
      }

      //Send
      sprintf(PUBLISH,
              "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
              PUB_TOPIC1, 3, QOS, DUP_FLAG, REMAIN_FLAG);
      A9G.println(PUBLISH);
      A9G.flush();
      A9G.println(payload[0]);
      A9G.flush();
      delay(100);
      _buffer = A9G.readString();
      Serial.println(PUBLISH);
      Serial.println(payload[0]);


      if (_buffer.indexOf("ERROR") >= 0) {
        Serial.println(_buffer.indexOf("ERROR"));
        Serial.println(_buffer);
        delay(2000);
      } else {
        sprintf(PUBLISH,
                "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
                PUB_TOPIC2, 3, QOS, DUP_FLAG, REMAIN_FLAG);
        A9G.println(PUBLISH);
        A9G.flush();
        A9G.println(payload[1]);
        A9G.flush();
        delay(100);
        _buffer = A9G.readString();
        Serial.println(PUBLISH);
        Serial.println(payload[1]);


        if (_buffer.indexOf("ERROR") >= 0) {
          Serial.println(_buffer.indexOf("ERROR"));
          Serial.println(_buffer);
          delay(2000);
        } else {
          sprintf(PUBLISH,
                  "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
                  PUB_TOPIC3, 3, QOS, DUP_FLAG, REMAIN_FLAG);
          A9G.println(PUBLISH);
          A9G.flush();
          A9G.println(payload[2]);
          A9G.flush();
          delay(100);
          _buffer = A9G.readString();
          Serial.println(PUBLISH);
          Serial.println(payload[2]);


          if (_buffer.indexOf("ERROR") >= 0) {
            Serial.println(_buffer.indexOf("ERROR"));
            Serial.println(_buffer);
            delay(2000);
          } else {
            sprintf(PUBLISH,
                    "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
                    PUB_TOPIC4, 3, QOS, DUP_FLAG, REMAIN_FLAG);
            A9G.println(PUBLISH);
            A9G.flush();
            A9G.println(payload[3]);
            A9G.flush();
            delay(100);
            _buffer = A9G.readString();
            Serial.println(PUBLISH);
            Serial.println(payload[3]);


            if (_buffer.indexOf("ERROR") >= 0) {
              Serial.println(_buffer.indexOf("ERROR"));
              Serial.println(_buffer);
              delay(2000);
            } else {
              sprintf(PUBLISH,
                      "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
                      PUB_TOPIC5, 1, QOS, DUP_FLAG, REMAIN_FLAG);
              A9G.println(PUBLISH);
              A9G.flush();
              A9G.println(payload[4]);
              A9G.flush();
              delay(100);
              _buffer = A9G.readString();
              Serial.println(PUBLISH);
              Serial.println(payload[4]);


              if (_buffer.indexOf("ERROR") >= 0) {
                Serial.println(_buffer.indexOf("ERROR"));
                Serial.println(_buffer);
                delay(2000);
              } else {
                // Disconnect From Broker
                last_millis = millis();
                do {
                  if (millis() - last_millis > 30000) {
                    break;
                  } else {
                    A9G.println(F("AT+MQTTDISCONN"));
                    A9G.flush();  //Wait for string to  be sent
                    Serial.println(F("AT+MQTTDISCONN"));
                    delay(100);
                    _buffer = A9G.readString();
                    if (_buffer.indexOf("OK") >= 0) {
                      Serial.println("OK");
                      break;
                    } else {
                      delay(2000);
                    }
                  }
                } while (1);
              }
            }
          }
        }
      }
      break;
    }
  } while (1);

  delay(5000);
}
