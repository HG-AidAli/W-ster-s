#include <Wire.h>
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#include <Servo.h>

#define FIREBASE_HOST "https://waxteras-8636d-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "ZDis5UX4SHG8Z5nWiHnTpcvcpLH3yJwcP4hY4U7u"
#define WIFI_SSID "Hitachigymnasiet_2.4"
#define WIFI_PASSWORD "mittwifiarsabra"

FirebaseData firebaseData;
Servo servot;

const byte MotorSpeed = D1; //ska vara 5 (antar jag, så står det i mina tidigare komentarer)
const byte MotorDir = D3; //0

int minspeed = 450;
int maxspeed = 1020;
int nospeed = 0;


void setup() {
  servot.attach(D2, 544, 2400);
  
  Serial.begin(115200);
  Wire.begin(4, 2);
  delay(5000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  spin_servo(45);
  delay(2000);
  spin_servo(90);
  delay(2000);
  spin_servo(0);

  pinMode(MotorDir, OUTPUT);
  digitalWrite (MotorDir, HIGH);
  
}

void sendingInfoToFirebase() {

    if (Firebase.setFloat(firebaseData, "/environment/temperature", 26.4)) {
      Serial.println("Temperature sent to Firebase successfully.");
    } else {
      Serial.print("Failed to send temperature to Firebase: ");
      Serial.println(firebaseData.errorReason());
    }

    if (Firebase.setFloat(firebaseData, "/environment/humidity", 30)) {
      Serial.println("Humidity sent to Firebase successfully.");
    } else {
      Serial.print("Failed to send humidity to Firebase: ");
      Serial.println(firebaseData.errorReason());
    }
  }


void spin_servo(int grader) {
  servot.write(grader);
}


void loop() {
  sendingInfoToFirebase();
  delay(5000);


  digitalWrite(MotorDir,HIGH);

  analogWrite(MotorSpeed, nospeed);
  delay(2000);
  analogWrite(MotorSpeed, minspeed);
  delay(5000);
  analogWrite(MotorSpeed, maxspeed);
  delay(5000);
  analogWrite(MotorSpeed, minspeed);
  delay(5000);
  analogWrite(MotorSpeed, nospeed);
  delay(2000);

  digitalWrite(MotorDir,LOW);
  
  analogWrite(MotorSpeed, nospeed);
  delay(2000);
  analogWrite(MotorSpeed, minspeed);
  delay(5000);
  analogWrite(MotorSpeed, maxspeed);
  delay(5000);
  analogWrite(MotorSpeed, minspeed);
  delay(5000);
  analogWrite(MotorSpeed, nospeed);
  delay(2000);
  
}
