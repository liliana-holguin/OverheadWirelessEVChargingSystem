/*
# Liliana Holguin
# NMSU
# SPRING 2025
# ENGR 402
# Capstone - Overhead Wireless EV Charging System
*/
#include <Wire.h>
#include <AccelStepper.h>

#define stepPin3 8
#define dirPin3 9
AccelStepper stepper3(1, stepPin3, dirPin3);

#define USwitch 3
#define DSwitch 2

int max = 600;
int speed = 600;
int acc = 800;

#define slaveYAddr 2

String receivedCmd = "";
bool newCmdReceived = false;

long targetY;
bool isDone = false;

void(*resetFunc)(void) = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("SLAVE Y-AXIS ESTABLISHING...");

  pinMode(USwitch, INPUT_PULLUP);
  pinMode(DSwitch, INPUT_PULLUP);

  stepper3.setMaxSpeed(max);
  stepper3.setAcceleration(acc);
  stepper3.enableOutputs();
  Serial.println("Stepper 3 Connected");

  Wire.begin(slaveYAddr);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  Serial.println("Slave Established");
}

void loop() {
  if (newCmdReceived) {
    Serial.print("SLAVE: Command received: ");
    Serial.println(receivedCmd);

    if (receivedCmd == "MOVEY") {
      moveY();
    } else if (receivedCmd == "HOMEY") {
      homeY();
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
  newCmdReceived = false;

  if (howMany == 2) {
    uint8_t lowByte = Wire.read();
    uint8_t highByte = Wire.read();
    targetY = (int16_t)((highByte << 8) | lowByte);
    Serial.print("SLAVE: Received targetY = ");
    Serial.println(targetY);
    return;
  }

  while (Wire.available()) {
    char c = Wire.read();
    receivedCmd += c;
  }
  newCmdReceived = true;
}

void moveY() {
  Serial.println("SLAVE: Moving Y...");

  stepper3.moveTo(targetY);

  while ((stepper3.distanceToGo() != 0) && (digitalRead(USwitch) == HIGH)) {
    stepper3.run();
  }

  if (digitalRead(USwitch) == LOW) {
    Serial.println("SLAVE: UP LIMIT SWITCH HIT");
  } else {
    Serial.println("SLAVE: TARGET HIT");
  }

  stepper3.stop();
  stepper3.setCurrentPosition(0);
  isDone = true;
}

void homeY() {
  Serial.println("SLAVE: Homing Y...");

  stepper3.setSpeed(speed);  
  while (digitalRead(USwitch) == HIGH) {
    stepper3.runSpeed();
  }
  Serial.println("SLAVE: UP LIMIT HIT");

  delay(200);
  stepper3.stop();

  stepper3.setSpeed(-speed); 
  while (digitalRead(DSwitch) == HIGH) {
    stepper3.runSpeed();
  }
  Serial.println("SLAVE: DOWN LIMIT HIT");

  delay(200); 
  stepper3.stop();

  stepper3.setCurrentPosition(0);
  isDone = true;

  Serial.println("SLAVE: HOMEY DONE");
}
