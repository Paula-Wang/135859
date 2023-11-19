/*	Arduino Tutorial - How to use a Soil Hygrometer Module
	Dev: Michalis Vasilakis // Date: 18/05/2016 // www.ardumotive.com	*/


//Constants 
const int hygrometer = A0;	//Hygrometer sensor analog pin output at pin A0 of Arduino
//Variables 
int value;

void setup(){
	
	Serial.begin(9600);
}

void loop(){
	
	// When the plant is watered well the sensor will read a value 380~400, I will keep the 400 
	// value but if you want you can change it below. 
	
	value = analogRead(hygrometer);		//Read analog value 
	value = constrain(value,330,560);	//Keep the ranges!
	value = map(value,330,560,100,0);	//Map value : 400 will be 100 and 1023 will be 0
	Serial.print("Soil humidity: ");
	Serial.print(value);
	Serial.println("%");
	delay(2000); //Read every 2 sec.
}
