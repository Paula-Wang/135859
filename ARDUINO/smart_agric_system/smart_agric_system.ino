
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;


//Uncomment the Library when using GPS module
//#include <TinyGPS++.h>

// Dht11 Library

// #include "DHT.h"

// GSM PINS

SoftwareSerial A9G(2, 3);  // RX, TX

// #define FONA_RX  2
// #define FONA_TX  3
// #define FONA_RST 4

// SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);


//Declaring DHT pins

// #define DHTPIN 5
// #define DHTTYPE DHT11

// DHT dht(DHTPIN, DHTTYPE);


//Defining Soil Capacitatve Moisture Sensor

// #define sensor A0
// int soilmoisturepercent;
// int soilMoistureValue;



// const int AirValue = 650;   
// const int WaterValue = 310;  


//Defining pins for GSM module. Uncomment the following lines when using GPS module

/**********************PINS FOR GSM MODULE***********************************
#define GPRS_RX  2
#define GPRS_TX  3

#define GPSBaud = 115200;

TinyGPSPlus gps;

SoftwareSerial gpsSerial(GPRS_RX, GPRS_TX);

double lattitude = gps.location.lat();
double longitude = gps.location.lng();

***************************************************************/


#define FONA_APN       "safaricom.co.ke"
// #define FONA_USERNAME  "saf"
// #define FONA_PASSWORD  "data"



#define AIO_SERVER       "io.adafruit.com"    // io.adafruit.com //156.0.232.201
#define AIO_SERVERPORT   1883
#define AIO_USERNAME    "Emphatic_frog2"       //keith_brian //iotlab
#define AIO_KEY         "aio_hYHf12CebgDmKdvDCEg43Lljo8Hw"    // aio_PUdO22MJJG6badFplIuhDLKNjgdL //80d18b0d

Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

// Temprature Values

Adafruit_MQTT_Publish temprature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temprature");

//Humidity Values

Adafruit_MQTT_Publish Humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity");

// Soil Moisture Values

Adafruit_MQTT_Publish moisture = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Moisture");

//GPS Data Values

//Adafruit_MQTT_Publish Location = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Location");

Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");



uint8_t txfailures = 0;
#define MAXTXFAILURES 3

void setup() {
  while (!Serial);
  
  Serial.begin(115200);

  Serial.println(F("Smart Agriculture Demo"));

  mqtt.subscribe(&onoffbutton);

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();

  //Initializing DHT11 sensor

  dht.begin();
  
  // Initialise the FONA module
  while (! FONAconnect(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD))) {
    Serial.println("Retrying FONA");
  }

  Serial.println(F("Connected to Cellular!"));

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
}


void loop() {
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  
  MQTT_connect();

  Watchdog.reset();

  // Reading Temprature, Humidity, Moisture & Location

  readDHT();
  delay(2000);
  readMoisture();
  delay(2000);
  //readGPRS();
  //delay(1000);

 

  Watchdog.reset();  
  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }


}


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}




//function to read Temprature and Humidity


void readDHT() {
 
  delay(2000);

  float humi  = dht.readHumidity();
  
  float tempC = dht.readTemperature();
 
  float tempF = dht.readTemperature(true);

  // check if any reads failed
  if (isnan(humi) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("%");

    Serial.print("  |  "); 

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print("°C ~ ");
    Serial.print(tempF);
    Serial.println("°F");
  }

  temprature.publish(tempC);
  Humidity.publish(humi);
 
}


//Function to read the Soil Moisture Levels

void readMoisture() {
  
   soilMoistureValue= analogRead(sensor);
  Serial.print("Moisture Value: ");
  Serial.print(soilMoistureValue);
  Serial.print(" ");
  Serial.print("--");
  
  int percent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if (percent < 0) {
    soilmoisturepercent = 0;
  }

  else if (percent > 100) {
    soilmoisturepercent = 100;
  }

  else if (percent >= 0 && percent <= 100) {
    soilmoisturepercent = percent;
  } 

 
  
  moisture.publish(float(percent));
  delay(10000);
}



// GPRS Module Read Lattitude and Longitude


// uncomment the following lines of codes when using the gsm module

/*****************GPS READ DATA********************************************

void readGPRS()
{
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      publishLocation();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected");
    while(true);
  }
}

void publishLocation()
{
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());

    Location.publish(lattitude,longitude);
  }
  
  else
  {
    Serial.println("Location: Not Available");
  }
  
}

*******************************************************************/
