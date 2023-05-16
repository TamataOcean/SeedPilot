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
#include <NTPClient.h>
#include <WiFiUdp.h>

#define ONE_WIRE_BUS 17
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds(&oneWire);
// variable to hold device addresses
DeviceAddress Thermometer;
int deviceCount = 0;

DynamicJsonDocument jsonDoc(256); 

const char* ssid     = "GasStationAP";
const char* password = "tamata50";
const char* mqtt_server = "172.24.1.1";
const char* mqtt_output = "esp32/seedpilot";
const char* mqtt_input = "esp32/input/seedpilot";
const char* mqtt_log = "esp32/log";
const char* mqtt_user = "ESP32_SeedPilot";
// TIME SERVER 
WiFiUDP ntpUDP;
long TIMEOFFSET = 2*60*60; // Regarding your position 0 for longon, +2 * 60min * 60sec Paris 
NTPClient timeClient(ntpUDP, TIMEOFFSET );
// SCHEDULING 
int hour;
int minute;
int seconde;
int sched_lightHourOn = 6;      // Light on at 6AM
int sched_lightHourOff= 22;     // Light off at 10PM

int relayHeater = 26; 
int relayLight = 25; 
int relayFan = 33; 
int relayPump = 32; 
bool stateHeater = false;
bool stateLight = false;
bool stateFan = false;
bool statePump = false;

// TEMPERATURE CONFIG
float seedTemperatureHigh = 25;
float seedTemperatureLow = 24.5;
float seedTemperatureMax = 25.5;

// FAN CONFIG
long fanInterval = 20000 ; //interval to activte each milliseconde the fan - every 5min
long fanActivation_duration = 60000; //during time Fan is activated

long lastMsg = 0;
long lastFanActivation = 0;
long lastWifiConnection = 0;
long lastConnection = 0; // Used to comeback to first loop if MQTT Reattemp connection is too long.
int maxAttempts = 10;
long timeInterval = 10000; //Mqtt send data every 20sec
long timeMaxWifi = 60000;
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

  // Init and get the time
  timeClient.begin();
  
  /* Getting all config on serial */
  printConfig();

}

/* ------------------------ */
/* ------ LOOP ------------ */
/* ------------------------ */
void loop() {
  long now = millis();
  client.loop();
  timeClient.update();

  hour = timeClient.getHours();
  minute = timeClient.getMinutes();
  seconde = timeClient.getSeconds();

  //Activate Light regarding schedTime
  if ( hour >= sched_lightHourOn && hour <= sched_lightHourOff && !stateLight) {
    digitalWrite(relayLight, HIGH);
    Serial.println("AUTO - Light on");
    stateLight = true;
  }
  else if ((hour <sched_lightHourOn || hour > sched_lightHourOff ) && stateLight ){
    digitalWrite(relayLight, LOW);
    Serial.println("AUTO - Light off");
    stateLight = false;
  }

  ds.requestTemperatures();
  double temperature = ds.getTempCByIndex(0);
  // Serial.print(temperature);
  // Serial.println( "°C");

  if ( temperature < seedTemperatureLow) {
    // Serial.println( " - Heater ON");
    digitalWrite(relayHeater, HIGH);
    stateHeater = true;
  }
  if ( temperature >= seedTemperatureHigh ) {
    // Serial.println(" - Heater Off");
    digitalWrite(relayHeater, LOW);
    stateHeater = false;
  }

  // Fan ON if temperature is too high 
  if ( temperature >= seedTemperatureMax ) {
    // Serial.println(" - Fan On");
    digitalWrite(relayFan, HIGH);
    lastFanActivation = now;
    stateFan = true;
    
  }
  // Fan OFF if temperature is too low
  if ( temperature <= seedTemperatureLow-1 ) {
    // Serial.println(" - Fan Off");
    digitalWrite(relayFan, LOW);
    stateFan = false;
  }
  //Fan activated no more than "duration" limit, UNLESS temperature is too high. 
  if ( now - lastFanActivation > fanActivation_duration && (temperature < seedTemperatureMax) ) {
    // Serial.println(" - Fan Off");
    digitalWrite(relayFan, LOW);
    stateFan = false;
  }

  //Schedule manager
  //schedRelay(now, temperature);

  // Every timeInterval, sending JSON data to Mqtt. 
  if (now - lastMsg > timeInterval ) {
    lastMsg = now;
    Serial.print("Temperature" + (String)temperature);
    Serial.println( "°C");
    String relayState ="";
    Serial.println();
    Serial.println("------ Relay State : ");

    if (stateHeater){relayState += "- Heater On\n";}else{relayState+="- Heater Off\n";}
    if (stateFan){relayState += "- Fan On\n";}else{relayState+="- Fan Off\n";}
    if (stateLight){relayState += "- Light On\n";}else{relayState+="- Light Off\n";}
    if (statePump){relayState += "- Pump On\n";}else{relayState+="- Pump Off\n";}
    
    Serial.println(relayState);
    
    if (!client.connected()) {
      Serial.println("Reconnect to Mqtt");
      reconnect();
    }
    String json = "{\"user\":\""+(String)mqtt_user+"\",\"Temperature\":\""+(String)temperature+"\",\"stateHeater\":"+stateHeater+",\"stateLight\":"+stateLight+",\"stateFan\":"+stateFan+",\"statePump\":"+statePump+"}";
    client.publish(mqtt_output, json.c_str());
    Serial.println("Mqtt sent to : " + (String)mqtt_output );
    Serial.println(json);
    delay(500);
    Serial.println(timeClient.getFormattedTime());
  }

  /* Incoming serial order ex: {"order":"INSTRUCTION"} */
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    Serial.print( " - message received : ");
    Serial.println(data);
    commandManager(data);
  }   
  delay(100);
}

/********************************/
/* Sceduler manager             */
/********************************/
void schedRelay(long now, double temperature){
  Serial.print("Now=");
  Serial.println(now);
  Serial.print("lastFanActivation=");
  Serial.println(lastFanActivation);
  Serial.print("fanInterval=");
  Serial.println(fanInterval);
  Serial.print("fanActivation_duration=");
  Serial.println(fanActivation_duration);
  
  // FAN SCHEDULER
  if ( (now - lastFanActivation > fanInterval) && (temperature > seedTemperatureLow ) ) {
    Serial.println("AUTO - Fan On");
    digitalWrite(relayFan, HIGH);
    lastFanActivation = now;
    stateFan = true;
  }

  if ( (now - lastFanActivation > fanActivation_duration) || (temperature <= seedTemperatureLow) ) {
    Serial.println("AUTO - duration is over, Fan Off");
    Serial.print("Now=");
    Serial.println(now);
    Serial.print("lastFanActivation=");
    Serial.println(lastFanActivation);
    digitalWrite(relayFan, LOW);
    stateFan = false;
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
  //  {"order":"Update_HourOn", "hour": 5}
  else if (jsonDoc["order"] == "Update_HourOn") {
    sched_lightHourOn = jsonDoc["hour"].as<int>();
    Serial.println("lightHourOn Changed with = "+ (String)sched_lightHourOn);
  }
  // {"order":"Update_HourOff", "hour": 22}
  else if (jsonDoc["order"] == "Update_HourOff") {
    sched_lightHourOff = jsonDoc["hour"].as<int>();
    Serial.println("lightHourOff Changed with = " + (String)sched_lightHourOff );
  }
  return 1;
  
}


/* ----------------------
 *  WIFI SETUP 
 *  ---------------------
 */
void setup_wifi() {
  long nowWifi = millis();
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    if ( nowWifi - lastWifiConnection > timeMaxWifi ) {
      lastWifiConnection = nowWifi;
      Serial.println("Max delay for wifi connection is over !");
      break;
    }
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
  // Loop until we're reconnected or 10 attemps to connect.
  int attempts = 0;
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
    attempts++;
    if (attempts > maxAttempts  ) {
      Serial.println("Impossible to connect after 10 attempts");
      Serial.println("Reboot... ");
      ESP.restart();
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

/* ------------------------
 *  PRINT CONFIG
 *  -----------------------
 */
void printConfig(){
  Serial.println("\n");
  Serial.println("------- WIFI CONFIG ----- ");
  Serial.println("-- SSID = " + (String)ssid);
  Serial.println("-- PWD = " + (String)password);
  Serial.println("-- MQTT SERVER = " + (String)mqtt_server);
  Serial.println("-- MQTT OUTPUT = " + (String)mqtt_output);
  Serial.println("-- MQTT INPUT = " + (String)mqtt_input);
  Serial.println("-- MQTT USER = " + (String)mqtt_user);
  Serial.println("-- MQTT MAX ATTEMPTS= " + (String)maxAttempts);
  Serial.println("\n");
  Serial.println("------- TEMPERATURE CONFIG ----- ");
  Serial.println("-- seed temperature high = " + (String)seedTemperatureHigh );
  Serial.println("-- seed temperature low = " + (String)seedTemperatureLow );
  Serial.println("-- seed temperature max = " + (String)seedTemperatureMax );
  Serial.println("\n");
  Serial.println("------- TIMER CONFIG ------ ");
  Serial.println("-- time Interval = " + (String)timeInterval );
  Serial.println("-- time Max for Wifi = " + (String)timeMaxWifi);
  Serial.println("\n");
  Serial.println("------- SCHED CONFIG ------ ");
  Serial.println("-- hour On = " + (String)sched_lightHourOn );
  Serial.println("-- hour Off = " + (String)sched_lightHourOff );
  
}
