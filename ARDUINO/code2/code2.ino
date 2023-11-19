#include <ArduinoJson.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Adafruit_Sensor.h>

#include <BH1750.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial A9G(2, 3);  // RX, TX
Adafruit_BME280 bme;
BH1750 lightMeter;

#define ONE_WIRE_BUS 9
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define LATITUDE -1.310095   //REMEMBER TO CHANGE THIS (USE GOOGLE MAPS)
#define LONGITUDE 36.813409  //REMEMBER TO CHANGE THIS (USE GOOGLE MAPS)

//PDP CONTEXT
const char *CID = "1";
const char *PDP_TYPE = "IPV6";
const char *APN = "sfctelematics";
char PDP_CONTEXT[64];

//MQTT CONNECTION
const char *HOST = "156.0.232.201";
const char *PORT = "1883";
const char *CLIENT_ID = "SU018J2023";  //REMEMBER TO CHANGE THIS (DEVICE NAME AS INDICATED ON THE CASING)
const char *ALIVE_SECONDS = "120";     //2 minutes
const char *CLEAN_SESSION = "0";
const char *QOS = "1";
const char *DUP_FLAG = "0";
const char *REMAIN_FLAG = "0";  // Changed the retain flag to 0/Flase
const char *USERNAME = "iotlab";
const char *PASSWORD = "80d18b0d";
char CONNECT[64];

//MQTT PUBLISH
const char *PUB_TOPIC = "iotlab/devices/AI4AFS-SU018J2023/telemetrys";  //REMEMBER TO CHANGE THIS

char PAYLOAD[64];
char PUBLISH[128];

String _buffer;
unsigned int interval = 1740;  // seconds
unsigned long int last_millis;
unsigned long int begin_millis;
unsigned long int end_millis;
int rssi_int = -113;

void (*resetFunc)(void) = 0;  // declare reset fuction at address 0

void setup() {
  //Disable SPI
  power_spi_disable();

  //Disable Serial(USART)
  //power_usart0_disable(); //Uncomment when uploading during final deployment and comment out all Serial communications

  // Disable ADC in ADCSRA
  ADCSRA = 0;
  // Disable ADC in PRR
  power_adc_disable();
  //Disable TWI
  power_twi_disable();

  //Wire.begin();
  //bme.begin(0x76);
  //lightMeter.begin();

  //Turn off brown-out enable in software
  MCUCR = bit(BODS) | bit(BODSE);
  MCUCR = bit(BODS);

  Serial.begin(115200);
  A9G.begin(115200);
  A9G.setTimeout(2000);
  _buffer.reserve(64);
  sensors.begin();

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

  //Turn GPS ON and OFF to turn off GPS LED
  //Turn GPS ON
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      resetFunc();  //call reset
    } else {
      A9G.println(F("AT+GPS=1"));
      A9G.flush();  //Wait for string to  be sent
      delay(100);
      Serial.println(F("AT+GPS=1"));
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        A9G.println(F("AT+GPS?"));
        A9G.flush();  //Wait for string to  be sent
        Serial.println(F("AT+GPS?"));
        delay(100);
        _buffer = A9G.readString();
        if (_buffer.indexOf("1") >= 0) {
          Serial.println("1");
          break;
        } else {
          delay(500);
          Serial.println("Retrying...");
        }
      }
    }
  } while (1);

  //TURN GPS OFF
  last_millis = millis();
  do {
    if (millis() - last_millis > 120000) {
      break;
    } else {
      A9G.println(F("AT+GPS=0"));
      A9G.flush();  //Wait for string to  be sent
      delay(100);
      Serial.println(F("AT+GPS=0"));
      _buffer = A9G.readString();
      if (_buffer.indexOf("OK") >= 0) {
        Serial.println("OK");
        break;
      } else {
        A9G.println(F("AT+GPS?"));
        A9G.flush();  //Wait for string to  be sent
        Serial.println(F("AT+GPS?"));
        delay(100);
        _buffer = A9G.readString();
        if (_buffer.indexOf("0") >= 0) {
          Serial.println("0");
          break;
        } else {
          delay(500);
          Serial.println("Retrying...");
        }
      }
    }
  } while (1);

  // ATTATCH
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

  //Enable TWI(SPI)
  power_twi_enable();
  Wire.begin();
  bme.begin(0x76);
  lightMeter.begin();

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
      // Check Signal Strength
      last_millis = millis();
      do {
        if (millis() - last_millis > 120000) {
          break;
        } else {
          A9G.println(F("AT+CSQ"));
          A9G.flush();  //Wait for string to  be sent
          Serial.print(F("AT+CSQ"));
          delay(100);
          _buffer = A9G.readString();
          if (_buffer.indexOf("OK") >= 0) {
            char rssi_char[3];
            String rssi_string = _buffer.substring(_buffer.indexOf(":") + 2, _buffer.lastIndexOf(","));
            rssi_string.substring(0, 2).toCharArray(rssi_char, sizeof(rssi_char));
            rssi_int = atoi(rssi_char);
            Serial.println(rssi_int);
            if (rssi_int != 99) {
              rssi_int = rssi_int * 2;
              rssi_int = rssi_int - 113;
            }
            Serial.println(rssi_int);
            Serial.println("OK");
            break;
          } else {
            delay(1000);
          }
        }
      } while (1);
      //Finish Signal Strength Check

      //Enable TWI(SPI)
      //power_twi_enable();
      //Wire.begin();
      //bme.begin(0x76);
      //lightMeter.begin();

      begin_millis = millis();
      //Read Sensors' Values
      float internal_temperature = bme.readTemperature();
      uint32_t pressure = bme.readPressure();
      float humidity = bme.readHumidity();
      int lux = lightMeter.readLightLevel();
      //Disable TWI(SPI)
      power_twi_disable();

      sensors.requestTemperatures();
      float external_temperature = sensors.getTempCByIndex(0);

      uint16_t voltage = getAccurateVcc();

      //Format to JSON
      StaticJsonDocument<256> doc;
      doc["lat"] = LATITUDE;
      doc["lng"] = LONGITUDE;
      doc["i_temp"] = internal_temperature;
      doc["pressure"] = pressure;
      doc["hum"] = humidity;
      doc["lux"] = lux;
      doc["e_temp"] = external_temperature;
      doc["volt"] = voltage;
      doc["rssi"] = rssi_int;
      char PAYLOAD[140];
      int _length = serializeJson(doc, PAYLOAD);
      //Send
      sprintf(PUBLISH,
              "AT+MQTTPUB=\"%s\",%d,%s,%s,%s",
              PUB_TOPIC, _length, QOS, DUP_FLAG, REMAIN_FLAG);
      A9G.println(PUBLISH);
      A9G.flush();
      A9G.println(PAYLOAD);
      A9G.flush();
      delay(100);
      _buffer = A9G.readString();
      Serial.println(PUBLISH);
      Serial.println(PAYLOAD);
      Serial.print("Length: ");
      Serial.println(_length);
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
      break;
    }

  } while (1);

  end_millis = millis();

  // sleep between sends
  for (int i = 0; i < (interval / 8); i++) {
    configurePowerSavingMode();
  }
}

//Voltage
int getAccurateVcc() {
  getVcc();
  return getVcc();
}

int getVcc(void) {
  int Vcc = 0;

  //Enable ADC
  power_adc_enable();

  ADCSRA = (1 << ADEN);
  ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
  ADMUX = (0 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);
  delay(1);  // wait for ADC and VREF to settle

  ADCSRA |= (1 << ADSC);  // start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;
  {}  // wait until done
  Vcc = ADC;

  // Disable ADC in ADCSRA
  ADCSRA = 0;
  // Disable ADC in PRR
  power_adc_disable();

  //1126400 = 1.1*1024*1000 = 1126400UL
  Vcc = 1126400UL / (unsigned long)Vcc;
  return Vcc;  // Vcc in millivolts
}

// watchdog interrupt
ISR(WDT_vect) {
  wdt_disable();  // disable watchdog
}

void configurePowerSavingMode() {
  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset
  WDTCSR = bit(WDCE) | bit(WDE);
  // set interrupt mode and an interval
  WDTCSR = bit(WDIE) | bit(WDP3) | bit(WDP0);  // set WDIE, and 8 seconds delay
  wdt_reset();                                 // reset watchdog

  // ready to sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  // cancel sleep as a precaution

  sleep_disable();
}
