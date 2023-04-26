#include "OneWire.h"
#include "DallasTemperature.h"
#include <ArduinoJson.h>

OneWire oneWire(10);
DallasTemperature ds(&oneWire);

DynamicJsonDocument jsonDoc(256); 

int relayHeater = 16; // Pin 16 for the Relay on ESP32

int seedTemperatureHigh = 23;
int seedTemperatureLow = 21;

bool relayStatus = false; //Relay off at the beginning

void setup() {
  Serial.begin(115200);  // définition de l'ouverture du port série
  ds.begin();          // sonde activée

  Serial.println("INIT RELAY");
  // Pin for relay module set as output
  pinMode(relayHeater, OUTPUT);
  digitalWrite(relayHeater, HIGH);
}

void loop() {
  ds.requestTemperatures();
  double temperature = ds.getTempCByIndex(0);
  Serial.print(temperature);
  Serial.println( "°C");

  
  if ( temperature < seedTemperatureLow && !relayStatus) {
    Serial.println( " - Heater ON");
    digitalWrite(relayHeater, LOW);
    relayStatus = true;
  }
  if ( temperature >= seedTemperatureHigh && relayStatus ) {
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
  
  delay(100);
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
  // {"order":"Update_seedTemperatureHigh", "NewTemperature":25}
  else if (jsonDoc["order"] == "Update_seedTemperatureHigh") {
    Serial.println( " - Update seedTemperatureHigh order received");
    seedTemperatureHigh = jsonDoc["seedTemperatureHigh"];
    Serial.print("seedTemperatureHigh set to : ");
    //Serial.println(jsonDoc["seedTemperatureHigh"]);
  }
  // {"order":"Update_seedTemperatureLow", "NewTemperature":18}
  else if (jsonDoc["order"] == "Update_seedTemperatureLow") {
    Serial.println( " - Update seedTemperatureLow order received");
    seedTemperatureHigh = jsonDoc["seedTemperatureLow"];
    Serial.print("seedTemperatureLow set to : ");
    //Serial.println(jsonDoc["seedTemperatureLow"]);
  }

  
}
