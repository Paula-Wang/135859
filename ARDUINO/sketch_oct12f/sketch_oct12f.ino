#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;
sensors_event_t temp_event, humidity_event; // Declare the sensors_event_t structs

#define trigpin 11
#define echopin 10
#define soilpin A0
#define relay 8
float moisture;
float duration, distance;
float temp;
float humidity;

void setup() {
  Serial.begin(9600);
  aht.begin();
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  pinMode(soilpin, INPUT);
  pinMode(relay, OUTPUT);
}

void loop() {
  // Use the library's methods to obtain temperature and humidity readings
  aht.getEvent(&temp_event, &humidity_event); // Pass two event variables

  temp = temp_event.temperature;
  humidity = humidity_event.relative_humidity;

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  moisture = analogRead(soilpin);
  moisture = constrain(moisture, 400, 1023);
  moisture = map(moisture, 400, 1023, 100, 0);

  digitalWrite(trigpin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigpin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigpin, LOW);
  duration = pulseIn(echopin, HIGH);
  distance = (duration / 2) * 0.0344;

  Serial.println("Moisture: " + String(moisture));
  Serial.println("Distance: " + String(distance) + " cm");

  if (moisture <= 30) {
    digitalWrite(relay, HIGH);
  } else if (moisture > 30) {
    if (digitalRead(relay) == 0) {
      digitalWrite(relay, LOW);
    } else if (digitalRead(relay) == 1) {
      if (moisture <= 60) {
        digitalWrite(relay, HIGH);
      } else if (moisture > 60) {
        digitalWrite(relay, LOW);
      }
    }
  }

  delay(5000);
}
