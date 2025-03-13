
int relayHeater = 26; 
int relayLight = 25; 
int relayFan = 32; 
int relayPump = 33; 

void setup() {
  // put your setup code here, to run once:

Serial.begin(115200);  // définition de l'ouverture du port série
  delay(10);
  Serial.println("INIT RELAY");
  // Pin for relay module set as output
  pinMode(relayHeater, OUTPUT);
  digitalWrite(relayHeater, HIGH);
  pinMode(relayLight, OUTPUT);
  digitalWrite(relayLight, HIGH);
  pinMode(relayFan, OUTPUT);
  digitalWrite(relayFan, HIGH);
  pinMode(relayPump, OUTPUT);
  digitalWrite(relayPump, HIGH);


}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Realy Heater ON");
  // Pin for relay module set as output
  digitalWrite(relayHeater, LOW);
  delay(5000);
  digitalWrite(relayHeater, HIGH);
  delay(1000);
  
  Serial.println("Realy Light ON");
  digitalWrite(relayLight, LOW);
  delay(5000);
  digitalWrite(relayLight, HIGH);
  delay(1000);
  
  Serial.println("Realy FAN ON");
  digitalWrite(relayFan, LOW);
  delay(5000);
  digitalWrite(relayFan, HIGH);
  delay(1000);
  
  Serial.println("Realy Pump ON");
  digitalWrite(relayPump, LOW);
  delay(5000);
  digitalWrite(relayPump, HIGH);
  delay(1000);


}
