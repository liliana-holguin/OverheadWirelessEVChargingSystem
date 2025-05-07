/*
# Liliana Holguin
# NMSU
# SPRING 2025
# ENGR 402
# Capstone - Overhead Wireless EV Charging System
*/
#include <Wire.h>
#include <AccelStepper.h>

#define stepPin1 8
#define dirPin1 9
AccelStepper stepper1(1, stepPin1, dirPin1);

#define stepPin2 10
#define dirPin2 11
AccelStepper stepper2(1, stepPin2, dirPin2);

#define RSwitch 3
#define LSwitch 2

int max = 600;
int speed = 600;
int acc = 800;

#define slaveXAddr 1

String receivedCmd = "";
bool newCmdReceived = false;

volatile int targetX = 8257;
bool isDone = false;

void(*resetFunc)(void) = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("SLAVE X-AXIS ESTABLISHING...");

  pinMode(LSwitch, INPUT_PULLUP);
  pinMode(RSwitch, INPUT_PULLUP);

  stepper1.setMaxSpeed(max);
  stepper1.setAcceleration(acc);
  stepper1.enableOutputs();
  Serial.println("Stepper 1 Connected");

  stepper2.setMaxSpeed(max);
  stepper2.setAcceleration(acc);
  stepper2.enableOutputs();
  Serial.println("Stepper 2 Connected");

  Wire.begin(slaveXAddr);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  Serial.println("Slave Established");
}

void loop() {
  if (newCmdReceived) {
    Serial.print("SLAVE: Command received: ");
    Serial.println(receivedCmd);

    // RESET if received
    if (receivedCmd == "RESET") {         
      Serial.println("SLAVE: RESETTING..."); 
      delay(100);                         
      resetFunc();
    }

    if (receivedCmd == "MOVEX") {
      moveX();
    } else if (receivedCmd == "HOMEX") {
      homeX();
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
  if (howMany < 1) return;

  char header = Wire.read(); 
  howMany--;

  if (header == 'C') {
    if (howMany == sizeof(int)) {
      Wire.readBytes((char*)&targetX, sizeof(targetX));
      Serial.print("Received int targetX = ");
      Serial.println(targetX);
    } else {
      Serial.println("ERROR: Expected 4 bytes for int, got something else.");
      while (Wire.available()) Wire.read(); 
    }
  } else {
    receivedCmd = String(header);
    while (Wire.available()) {
      receivedCmd += (char)Wire.read();
    }
    newCmdReceived = true;
    Serial.print("Received command: ");
    Serial.println(receivedCmd);
  }
}


void moveX() {
  Serial.print("Moving to targetX = ");
  Serial.println(targetX);

  stepper1.moveTo(targetX);
  stepper2.moveTo(-targetX);

  while ((stepper1.distanceToGo() != 0 && stepper2.distanceToGo() != 0) && digitalRead(LSwitch) == HIGH) {
    
    stepper1.run();
    stepper2.run();
  }

  if (digitalRead(LSwitch) == LOW) {
    Serial.println("Right limit switch triggered.");
  } else {
    Serial.println("Target position reached.");
  }

  stepper1.stop();
  stepper2.stop();

  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);

  isDone = true;
}


void homeX() {
  stepper1.setSpeed(-speed);
  stepper2.setSpeed(speed);

  while (digitalRead(RSwitch) == HIGH) {
    stepper1.runSpeed();
    stepper2.runSpeed();
  }

  stepper1.stop();
  stepper2.stop();

  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);

  Serial.println("DONE");

  isDone = true;
}
