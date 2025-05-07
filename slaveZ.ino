/*
# Liliana Holguin
# NMSU
# SPRING 2025
# ENGR 402
# Capstone - Overhead Wireless EV Charging System
*/
#include <Wire.h>

#define RPWM 5 
#define LPWM 6
#define R_EN 7 
#define L_EN 8

#define trigPin 13
#define echoPin 12

float distance();
void extendActuator();
void retractActuator();
void stopActuator();
void moveZ();

#define slaveZAddr 3

int sec = 500;
bool isDone = false;
String receivedCmd = "";
bool newCmdReceived = false;

void(*resetFunc)(void) = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("SLAVE Z-AXIS ESTABLISHING...");

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Wire.begin(slaveZAddr);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  Serial.println("Z Slave Established");

  retractActuator();
}

void loop() {
  
  if (newCmdReceived) {
    Serial.print("SLAVE: Command received: ");
    Serial.println(receivedCmd);

    if (receivedCmd == "RESET") {         
      Serial.println("SLAVE: RESETTING...");  
      delay(100);                         
      resetFunc();                        
    }

    if (receivedCmd == "MOVEZ") {
      moveZ();
    }

    receivedCmd = "";
    newCmdReceived = false;
  }
}

void requestEvent() {
  if (isDone) {
    Wire.write("DONE");
    isDone = false;
  } else {
    Wire.write("BUSY");
  }
}

void receiveEvent(int howMany) {
  receivedCmd = "";
  while (Wire.available()) {
    char c = Wire.read();
    receivedCmd += c;
  }
  newCmdReceived = true;
}

void moveZ() {
  Serial.println("SLAVE: Moving Z...");
  float dis = distance();
  int count = 0;

  while (dis > 5) {
    extendActuator();
    delay(sec);
    count++;
    dis = distance();
  }

  stopActuator();
  delay(3000);
  retractActuator();

  isDone = true;
}

float distance() {
  float sum = 0;

  for (int i = 0; i < 3; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);
    float dist = duration * 0.034 / 2;
    sum += dist;
    delay(10);
  }

  float avg = sum / 3.0;
  Serial.print("Distance (cm): ");
  Serial.println(avg);
  return avg;
}

void extendActuator() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 200);
}

void retractActuator() {
  analogWrite(RPWM, 200);
  analogWrite(LPWM, 0);
}

void stopActuator() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}
