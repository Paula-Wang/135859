#include <Adafruit_Sensor.h>
// #include <DHT.h>
// #include <DHT_U.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;


#define trigpin 11
#define echopin 10
#define soilpin A0
#define relay 8
// #define dhtpin 7
// #define DHTTYPE DHT11
float temp;
float humidity;
float moisture;
float duration, distance;

// DHT_Unified dht(dhtpin, DHTTYPE);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  pinMode(soilpin, INPUT);
  pinMode(relay, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temp = event.temperature;

  // Get humidity event and print its moisture.
  dht.humidity().getEvent(&event);
  humidity = event.relative_humidity;

  // Get soil moisture
  moisture = analogRead(soilpin);		//Read analog moisture 
	moisture = constrain(moisture,400,1023);	//Keep the ranges!
	moisture = map(moisture,400,1023,100,0);	//Map moisture : 400 will be 100 and 1023 will be 0


  //Get water level
  digitalWrite(trigpin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigpin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigpin, LOW);

  duration = pulseIn(echopin, HIGH);
  distance = (duration / 2) * 0.0344;

  Serial.println(temp);
  Serial.println(humidity);
  Serial.println(moisture);
  Serial.println(distance);
  Serial.println("");

  if (moisture<=30)
  {
    //ON
    digitalWrite(relay, HIGH);
  }
  else if (moisture>30)
  {
    //check pump state
    if (digitalRead(relay) == 0) 
    {
      //pump = 0
      digitalWrite(relay, LOW);
    }
    else if (digitalRead(relay) == 1)
    {
      if (moisture <= 60)
      {
        //pump = on
        digitalWrite(relay, HIGH);
      }
      else if (moisture > 60)
      {
        //pump = off
        digitalWrite(relay, LOW);
      }
    }
  }
  
  delay(5000);
}