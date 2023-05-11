/*
* Description:
* This sample code is mainly used to monitor Temperature for SeedControl
* and use relay to activate Heater, Light, Pump & fan. 
* Software Environment: Arduino IDE 1.8.9
*
* Install the library file：
* Copy the files from the github repository folder libraries to the libraries
* in the Arduino IDE 1.8.9 installation directory
*
* Hardware platform   : ESP32
*
* author  :  Rominco(rtourte@yahoo.fr)
* version :  V1.0
* date    :  2023-05-10
**********************************************************************
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include <ArduinoJson.h>

#define ONE_WIRE_BUS 17
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds(&oneWire);
// variable to hold device addresses
DeviceAddress Thermometer;
int deviceCount = 0;

DynamicJsonDocument jsonDoc(256); 

const char* ssid     = "WifiRaspi";
const char* password = "wifiraspi";
const char* mqtt_server = "172.24.1.1";
const char* mqtt_output = "esp32/update/seedpilot";
const char* mqtt_input = "esp32/input/seedpilot";
const char* mqtt_log = "esp32/log";
const char* mqtt_user = "ESP32_SeedPilot";

int relayHeater = 26; 
int relayLight = 25; 
int relayFan = 33; 
int relayPump = 32; 
bool stateHeater = false;
bool stateLight = false;
bool stateFan = false;
bool statePump = false;


int seedTemperatureHigh = 25;
int seedTemperatureLow = 24.5;
long lastMsg = 0;
int timeInterval = 7500;

bool relayStatus = false; //Relay off at the beginning

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
int value = 0;

/* ------------------------
 *  MAIN SETUP 
 *  -----------------------
 */
void setup() {
  Serial.begin(115200);  // définition de l'ouverture du port série
  delay(10);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("DS BEGIN");
  ds.begin();          // sonde activée

  Serial.println("INIT RELAY");
  // Pin for relay module set as output
  pinMode(relayHeater, OUTPUT);
  digitalWrite(relayHeater, LOW);
  pinMode(relayLight, OUTPUT);
  digitalWrite(relayLight, LOW);
  pinMode(relayFan, OUTPUT);
  digitalWrite(relayFan, LOW);
  pinMode(relayPump, OUTPUT);
  digitalWrite(relayPump, LOW);
}

void loop() {

  long now = millis();
  
  client.loop();

  ds.requestTemperatures();
  double temperature = ds.getTempCByIndex(0);
  Serial.print(temperature);
  Serial.println( "°C");

  if ( temperature < seedTemperatureLow) {
    Serial.println( " - Heater ON");
    digitalWrite(relayHeater, HIGH);
    stateHeater = true;
  }
  if ( temperature >= seedTemperatureHigh ) {
        Serial.println(" - Heater Off");
    digitalWrite(relayHeater, LOW);
    Serial.println("OFF");
    stateHeater = false;
  }

  if (now - lastMsg > timeInterval ) {
    lastMsg = now;
    if (!client.connected()) {
      Serial.println("Reconnect to Mqtt");
      reconnect();
    }
    String json = "{\"user\":\""+(String)mqtt_user+"\",\"Temperature\":\""+(String)temperature+"\",\"stateHeater\":"+stateHeater+",\"stateLight\":"+stateLight+",\"stateFan\":"+stateFan+",\"statePump\":"+statePump+"}";
    client.publish(mqtt_output, json.c_str() );
    //client.disconnect();
    Serial.println("Mqtt sent to : " + (String)mqtt_output );
    Serial.println(json);
    delay(500);
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
    digitalWrite(relayHeater, HIGH);
    Serial.println("HeaterOn");
    stateHeater = true;
  }
  else if (jsonDoc["order"] == "HeaterOff") {
    Serial.println( " - Heater Off order received");
    digitalWrite(relayHeater, LOW);
    Serial.println("HeaterOff");
    stateHeater = false;
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
  // {"order":"LightOff"}
  else if (jsonDoc["order"] == "LightOff") {
    Serial.println( " - Light Off order received");
    digitalWrite(relayLight, LOW);
    Serial.println("LightOff");
    stateLight = false;
  }
  // {"order":"LightOn"}
  else if (jsonDoc["order"] == "LightOn") {
    Serial.println( " - Light On order received");
    digitalWrite(relayLight, HIGH);
    Serial.println("LightOn");
    stateLight = true;
  }
  // {"order":"FanOff"}
  else if (jsonDoc["order"] == "FanOff") {
    Serial.println( " - Fan Off order received");
    digitalWrite(relayFan, LOW);
    Serial.println("FanOff");
    stateFan = false;
  }
  // {"order":"FanOn"}
  else if (jsonDoc["order"] == "FanOn") {
    Serial.println( " - Fan On order received");
    digitalWrite(relayFan, HIGH);
    Serial.println("FanOn");
    stateFan = true;
  }
  // {"order":"PumpOff"}
  else if (jsonDoc["order"] == "PumpOff") {
    digitalWrite(relayPump, LOW);
    Serial.println( " - pump Off order received");
    Serial.println("PumpOff");
    statePump = false;
  }
  // {"order":"PumpOn"}
  else if (jsonDoc["order"] == "PumpOn") {
    Serial.println( " - pump On order received");
    digitalWrite(relayPump, HIGH);
    Serial.println("PumpOn");
    statePump = true;
  }
  return 1;
  
}


/* ----------------------
 *  WIFI SETUP 
 *  ---------------------
 */
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/* 
 *  ------------------
 *  RECONNECT MQTT
 *  ------------------
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    delay(100);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(mqtt_input);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
}

/* ------------------------
 *  MQTT CALLBACK
 *  -----------------------
 */
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == mqtt_input ) {
    commandManager(messageTemp);
  }
}
