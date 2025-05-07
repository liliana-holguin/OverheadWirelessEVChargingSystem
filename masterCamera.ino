/*
# Liliana Holguin
# NMSU
# SPRING 2025
# ENGR 402
# Capstone - Overhead Wireless EV Charging System
*/
#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include "memorysaver.h"

#if !(defined OV2640_MINI_2MP)
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define CS 7
bool is_header = false;
int mode = 0;
uint8_t start_capture = 0;

#if defined(OV2640_MINI_2MP)
ArduCAM myCAM(OV2640, CS);
#else
ArduCAM myCAM(OV5642, CS);
#endif

#define slaveXAddr 1
#define slaveYAddr 2
#define slaveZAddr 3

int moveState = 0;
String inputString = "";
bool stringComplete = false;
bool readyToMove = false;

void setupCam();
uint8_t read_fifo_burst(ArduCAM myCAM);
void home();

void setup() {
  Wire.begin();
  Serial.begin(921600);
  setupCam();
}

void loop() {
  if (Serial.available()) {
    uint8_t cmd = Serial.read();
    if (cmd == 0x10) {
      start_capture = 1;
      mode = 1;
      delay(100);
    }
    else if (cmd == "RESET") {
      Wire.beginTransmission(slaveXAddr);
      Wire.write("RESET");  
      Wire.endTransmission();

      Wire.beginTransmission(slaveYAddr);
      Wire.write("RESET");  
      Wire.endTransmission();

      Wire.beginTransmission(slaveZAddr);
      Wire.write("RESET"); 
      Wire.endTransmission();

    }
  }

  if (mode == 1 && start_capture == 1) {
    inputString = "";         
    stringComplete = false;
    myCAM.flush_fifo();
    myCAM.clear_fifo_flag();
    myCAM.start_capture();
    start_capture = 0;
  }

  if (mode == 1 && myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    Serial.println(F("ACK CMD CAM Capture Done. END"));
    delay(50);
    read_fifo_burst(myCAM);
    myCAM.clear_fifo_flag();
    mode = 0;
    inputString = "";         
    stringComplete = false;
  }

  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      break;
    }
    inputString += inChar;
  }

  if (stringComplete) {
    int commaIndex = inputString.indexOf(',');
    if (commaIndex != -1) {
      String xString = inputString.substring(0, commaIndex);
      String yString = inputString.substring(commaIndex + 1);

      int xCoord = xString.toInt();
      int yCoord = yString.toInt();

      Serial.print("X: ");
      Serial.print(xCoord);
      Serial.print(" Y: ");
      Serial.println(yCoord);


      Wire.beginTransmission(slaveXAddr);
      Wire.write('C'); 
      Wire.write((uint8_t*)&xCoord, sizeof(xCoord));
      Wire.endTransmission(true);
      
      while (Serial.available()) {
        Serial.read();  // clear garbage
      }


      Wire.beginTransmission(slaveYAddr);
      Wire.write((uint8_t*)&yCoord, sizeof(yCoord));  // sends 4 bytes
      Wire.endTransmission();

      readyToMove = true;
    }
    inputString = "";
    stringComplete = false;
  }


  if (moveState == 0 && mode == 0 && readyToMove) {
    moveState = 1;
    readyToMove = false;
  } else if (moveState == 1) {
    Wire.beginTransmission(slaveXAddr);
    Wire.write("MOVEX");
    Wire.endTransmission();
    String status = "";
    do {
      status = "";
      Wire.requestFrom(slaveXAddr, 4);
      while (Wire.available()) status += (char)Wire.read();
    } while (status == "BUSY");
    if (status == "DONE") {
      Wire.beginTransmission(slaveYAddr);
      Wire.write("MOVEY");
      Wire.endTransmission();
      moveState = 2;
    }
  } else if (moveState == 2) {
    String status = "";
    do {
      status = "";
      Wire.requestFrom(slaveYAddr, 4);
      while (Wire.available()) status += (char)Wire.read();
    } while (status == "BUSY");
    if (status == "DONE") {
      Wire.beginTransmission(slaveZAddr);
      Wire.write("MOVEZ");
      Wire.endTransmission();
      moveState = 3;
    }
  } else if (moveState == 3) {
    String status = "";
    do {
      status = "";
      Wire.requestFrom(slaveZAddr, 4);
      while (Wire.available()) status += (char)Wire.read();
    } while (status == "BUSY");
    if (status == "DONE") {
      moveState = 0;
    }
  } else if (moveState == 4) {
    home();
    moveState = 0;
  }
}

void setupCam() {
  uint8_t vid, pid, temp;
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();

  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);
  while (true) {
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp == 0x55) break;
    delay(1000);
  }

#if defined(OV2640_MINI_2MP)
  while (true) {
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid == 0x26) && (pid == 0x41 || pid == 0x42)) break;
    delay(1000);
  }
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#else
  while (true) {
    myCAM.wrSensorReg16_8(0xff, 0x01);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if ((vid == 0x56) && (pid == 0x42)) break;
    delay(1000);
  }
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.OV5642_set_JPEG_size(OV5642_320x240);
#endif

  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  delay(1000);
  myCAM.clear_fifo_flag();
#if !(defined(OV2640_MINI_2MP))
  myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);
#endif
}

uint8_t read_fifo_burst(ArduCAM myCAM) {
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = myCAM.read_fifo_length();
  if (length >= MAX_FIFO_SIZE || length == 0) return 0;
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  temp = SPI.transfer(0x00);
  length--;
  while (length--) {
    temp_last = temp;
    temp = SPI.transfer(0x00);
    if (is_header) {
      Serial.write(temp);
    } else if (temp == 0xD8 && temp_last == 0xFF) {
      is_header = true;
      Serial.write(temp_last);
      Serial.write(temp);
    }
    if (temp == 0xD9 && temp_last == 0xFF) break;
    delayMicroseconds(15);
  }
  myCAM.CS_HIGH();
  is_header = false;
  return 1;
}

void home() {
  static int step = 1;
  String status = "";
  if (step == 1) {
    Wire.beginTransmission(slaveXAddr);
    Wire.write("HOMEX");
    Wire.endTransmission();
    do {
      status = "";
      Wire.requestFrom(slaveXAddr, 4);
      while (Wire.available()) status += (char)Wire.read();
    } while (status == "BUSY");
    if (status == "DONE") {
      Wire.beginTransmission(slaveYAddr);
      Wire.write("HOMEY");
      Wire.endTransmission();
      step = 2;
    }
  } else if (step == 2) {
    do {
      status = "";
      Wire.requestFrom(slaveYAddr, 4);
      while (Wire.available()) status += (char)Wire.read();
    } while (status == "BUSY");
    if (status == "DONE") {
      step == 1;
    }
  }
}