#include "OneWire.h"
#include "DallasTemperature.h"
#include <ArduinoJson.h>

OneWire oneWire(10);
DallasTemperature ds(&oneWire);

DynamicJsonDocument jsonDoc(256); 

int relayHeater = 33; // Pin 33 for the Relay on Teensy

double seedTemperatureHigh = 25;
double seedTemperatureLow = 24.6;
int timeInterval = 7500;
long lastMsg = 0;

bool relayStatus = false; //Relay off at the beginning

void setup() {
  Serial.begin(115200);  // définition de l'ouverture du port série
  ds.begin();          // sonde activée

  delay(30);
  Serial.println("INIT RELAY");
  Serial.print("Temperature High = ");
  Serial.println(seedTemperatureHigh );
  Serial.print("Temperature Low = ");
  Serial.println(seedTemperatureLow );
  
  // Pin for relay module set as output
  pinMode(relayHeater, OUTPUT);
  digitalWrite(relayHeater, HIGH);
}

void loop() {
  long now = millis();
  if (now - lastMsg > timeInterval ) {
    lastMsg = now;
    
    ds.requestTemperatures();
    double temperature = ds.getTempCByIndex(0);
    Serial.print(temperature);
    Serial.println( "°C");
  
    
    if ( temperature < seedTemperatureLow ) {
      Serial.println( " - Heater ON");
      digitalWrite(relayHeater, LOW);
      relayStatus = true;
    }
    if ( temperature >= seedTemperatureHigh ) {
          Serial.println(" - Heater Off");
      digitalWrite(relayHeater, HIGH);
      Serial.println("OFF");
      relayStatus = false;
    }
    
    /* Gestion des entrées en commande {"order":"INSTRUCTION"} */
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      Serial.print( " - message received : ");
      Serial.println(data);
      commandManager(data);
    }      
  }
}


/********************************/
/* Management Command order     */
/********************************/
int commandManager(String message) {
  DeserializationError error = deserializeJson(jsonDoc, message);
  if(error) {
    Serial.println("parseObject() failed");
  }
  // {"order":"HeaterOn"}
  if (jsonDoc["order"] == "HeaterOn") {
    Serial.println( " - Heater On order received");
    digitalWrite(relayHeater, LOW);
    Serial.println("HeaterOn");
  }
  else if (jsonDoc["order"] == "HeaterOff") {
    Serial.println( " - Heater Off order received");
    digitalWrite(relayHeater, HIGH);
    Serial.println("HeaterOff");
  }
  
}
