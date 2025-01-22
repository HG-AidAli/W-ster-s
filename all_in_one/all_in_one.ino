#include <Wire.h>
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <Servo.h>
#include <AM2320.h>

#define FIREBASE_HOST "https://waxteras-8636d-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "ZDis5UX4SHG8Z5nWiHnTpcvcpLH3yJwcP4hY4U7u"
#define WIFI_SSID "Hitachigymnasiet_2.4"
#define WIFI_PASSWORD "mittwifiarsabra"

const byte MotorSpeed = D1; 
const byte MotorDir = D3;
const byte PumpSpeed = D2;
const byte PumpDir = D4;
const byte ServoPin = D5; 

FirebaseData firebaseData;
AM2320 sensor;
Servo servot;

float SensorTemp;
float SensorHum;

unsigned long previousMillisFirebase = 0;
const unsigned long firebaseInterval = 4000;

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 2);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  pinMode(MotorDir, OUTPUT);
  digitalWrite(MotorDir, HIGH);
  
  servot.attach(ServoPin);
  Serial.println("Servo attached to pin D5");

  servot.write(90);
  delay(1000);
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
        // fetching pump speed here (there is also a bug here (prob))
int fetchPumpSpeed() {
  if (Firebase.getInt(firebaseData, "/pump/speed")) {
    return firebaseData.intData();
  }
  Serial.println("Error fetching pump speed.");
  return 0;
}
          // FEtching pump direction here (theres a bug)
bool fetchPumpDirection() {
  if (Firebase.getBool(firebaseData, "/pump/direction")) {
    return firebaseData.boolData();
  }
  Serial.println("Error fetching motor direction.");
  return false;
}

int fetchServoAngle() {
  if (Firebase.getInt(firebaseData, "/servo/angle")) {
    return firebaseData.intData();
  }
  Serial.println("Error fetching servo angle.");
  return 90;
}

void controlMotor(bool riktning, int hastighet) {
  if (hastighet < 0) hastighet = 0;
  if (hastighet == 0) hastighet = 0;
  if (hastighet !=0 && hastighet < 500) hastighet = 500;
  if (hastighet > 3000) hastighet = 3000;
  digitalWrite(MotorDir, riktning ? HIGH : LOW);
  analogWrite(MotorSpeed, hastighet);
}

void controlPump(bool riktning, int hastighet) {
  pinMode(PumpDir, OUTPUT);
  
  if (hastighet <= 0) {
    analogWrite(PumpSpeed, 0); // Stäng av pumpen
    return;
  }

  digitalWrite(PumpDir, riktning ? HIGH : LOW);
  analogWrite(PumpSpeed, hastighet);
}


void controlServo(int grader) {
  grader = constrain(grader, 0, 180);
  servot.write(grader*2);
}

void getTempHumAndSendToFirebase() {
  if (sensor.measure()) {
    SensorTemp = sensor.getTemperature  ();
    SensorHum = sensor.getHumidity();

    Serial.println("Temp: " + String(SensorTemp) + "°C, Hum: " + String(SensorHum) + "%");

    if (!Firebase.setFloat(firebaseData, "/environment/temperature", SensorTemp)) {
      Serial.println("Failed to send temperature: " + firebaseData.errorReason());
    }
    if (!Firebase.setFloat(firebaseData, "/environment/humidity", SensorHum)) {
      Serial.println("Failed to send humidity: " + firebaseData.errorReason());
    }

    if (SensorTemp > 27) {
      Serial.println("Warning: Temperature exceeds threshold!");
    }
  } else {
    Serial.println("Sensor Error: " + String(sensor.getErrorCode()));
  }
}

void sendCheck() {
  if (!Firebase.setBool(firebaseData, "/checker/check", true)) {
    Serial.println("Failed to send checker: " + firebaseData.errorReason());
  }
}

void loop() {
  controlPump(fetchPumpDirection(), fetchPumpSpeed());
  controlMotor(fetchMotorDirection(), fetchMotorSpeed());
  controlServo(fetchServoAngle());


  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisFirebase >= firebaseInterval) {
    previousMillisFirebase = currentMillis;

    getTempHumAndSendToFirebase();
    sendCheck();
  }

  yield();
}
