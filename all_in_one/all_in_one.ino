//This thing is PROBABLY functional... Fix any bug that you find

#include <Servo.h>
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <Wire.h>
#include <AM2320.h>

#define FIREBASE_HOST "https://waxteras-8636d-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "ZDis5UX4SHG8Z5nWiHnTpcvcpLH3yJwcP4hY4U7u"
#define WIFI_SSID "Hitachigymnasiet_2.4" 
#define WIFI_PASSWORD "mittwifiarsabra"

const byte MotorSpeed = D1;
const byte MotorDir = D3;
const byte PumpSpeed = D2;
const byte PumpDir = D4;
const byte ServoPin = D0;

Servo servot;
FirebaseData firebaseData;
AM2320 sensor;

float SensorTemp;
float SensorHum;

unsigned long previousMillisFirebase = 0;
const unsigned long firebaseInterval = 4000;

void setup() {
  Serial.begin(115200);
  Wire.begin(D5, D6);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  if (Firebase.setString(firebaseData, "/test", "connected")) {
      Serial.println("Firebase initialized successfully.");
  } else {
      Serial.println("Failed to initialize Firebase. Error: " + firebaseData.errorReason());
  }
  
  servot.attach(ServoPin);
}

int fetchMotorSpeed() {
  if (Firebase.getInt(firebaseData, "/motor/speed")) {
    return firebaseData.intData();
  }
  Serial.println("Error fetching motor speed.");
  return 0;
}

bool fetchMotorDirection() {
  if (Firebase.getBool(firebaseData, "/motor/direction")) {
    return firebaseData.boolData();
  }
  Serial.println("Error fetching motor direction.");
  return false;
}


int fetchServoAngle(){
  if (Firebase.getInt(firebaseData, "/servo/angle")) {
    return firebaseData.intData();
  }
  Serial.println("Error fetching SERVO angle.");
  return 0;
}


int fetchPumpSpeed() {
  if (Firebase.getInt(firebaseData, "/pump/speed")) {
    return firebaseData.intData();
  }
  Serial.println("Error fetching pump speed.");
  return 0;
}

bool fetchPumpDirection() {
  if (Firebase.getBool(firebaseData, "/pump/direction")) {
    return firebaseData.boolData();
  }
  Serial.println("Error fetching pump direction.");
  return false;
}

void controlMotor(bool riktning, int hastighet) {
  pinMode(MotorSpeed, OUTPUT);
  pinMode(MotorDir, OUTPUT);

  hastighet = constrain(hastighet, 0, 3000);

  digitalWrite(MotorDir, riktning ? HIGH : LOW);
  analogWrite(MotorSpeed, hastighet);
}

void controlPump(bool riktning, int hastighet) {
  
  pinMode(PumpSpeed, OUTPUT);
  pinMode(PumpDir, OUTPUT);

  hastighet = constrain(hastighet, 0, 3000);

  digitalWrite(PumpDir, riktning ? HIGH : LOW);
  analogWrite(PumpSpeed, hastighet);
}

void controlServo(int grader) {
  grader = constrain(grader, 0, 180);
  servot.write(grader * 2);
}

void getTempHumAndSendToFirebase(){
  if (sensor.measure()) {
    SensorTemp = sensor.getTemperature();
    SensorHum = sensor.getHumidity();

    if (!Firebase.setFloat(firebaseData, "/enviroment/temperature", SensorTemp)) {
      Serial.println("Failed to send temperature to firebase");     
    }
    if (!Firebase.setFloat(firebaseData, "/enviroment/humidity", SensorHum)) {
      Serial.println("Failed to send humidity to firebase");     
    }
    
  }

  else {
    Serial.println("Failed to fetch Sensory data from the sensors");
  }
}


void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisFirebase >= firebaseInterval) {
    previousMillisFirebase = currentMillis;

    getTempHumAndSendToFirebase();
  }

  controlPump(fetchPumpDirection(), fetchPumpSpeed());
  controlMotor(fetchMotorDirection(), fetchMotorSpeed());
  controlServo(fetchServoAngle());
  
  delay(300);

  yield();
}
