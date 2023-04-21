#include "motorDriver.h"
#include "shiftRegister.h"
//#include "debugLeds.h"
#include "external.h"
#include "altSPI.h"
#include "config.h"

#include <QTRSensors.h>

#include <math.h>
#include <string.h>
#include <inttypes.h>

short int eHasArrived=0;
unsigned long timeOfArrivalOfE;

uint16_t sensorValues[cfg::k_sensorCount] = { 0 };
QTRSensors qtr = QTRSensors();

ShiftRegister sr = ShiftRegister(cfg::pins::SRData, cfg::pins::SRClock, cfg::pins::SRStrobe);

MotorDriver driver = MotorDriver(&sr, cfg::pins::leftMotorPWM, cfg::srOrders::left1, cfg::srOrders::left2, 
                                      cfg::pins::rightMotorPWM, cfg::srOrders::right1, cfg::srOrders::right2);

//DebugLeds leds = DebugLeds(&sr, cfg::srOrders::LEDRed, cfg::srOrders::LEDGreen, cfg::srOrders::LEDBlue);

AltSPI spi = AltSPI(cfg::k_SPIConfigByte);


namespace {
  uint32_t lastTime = 1;
  uint32_t currentTime = 2;
  uint32_t dT = 1;

  int16_t position = 0;
  int16_t lastError = 0;
  int16_t currentError = 0;

  int16_t PIDSpeedModifier = 0.f;
  #ifdef DEBUG_SERIAL
    char* serialPrintBuffer = new char[250];
  #endif
}

namespace {
  uint16_t numberOfCommands;
  char* commands = nullptr;
  uint16_t commandCounter = 0;

  volatile uint8_t lastLineColorFlag = 1;
  volatile uint8_t lineColorFlag = 1; //0 FOR BLACK, 1 FOR WHITE
  volatile uint32_t lineColorTime = 1;

  volatile uint8_t lastOnLineFlag = 1;
  volatile uint8_t onLineFlag = 1;

  uint8_t runOnceFlag = 0;

  uint8_t junctionDoubleTestFlag = 0;
}

void setup() {
  //SETUP
    //SETUP PINS
      pinMode(cfg::pins::emitter, OUTPUT); 
      for(uint8_t i = 0; i < cfg::k_sensorCount; i++) {
        pinMode(cfg::pins::qtr[i], INPUT);
      } 

      pinMode(cfg::pins::SRClock, OUTPUT); 
      pinMode(cfg::pins::SRData, OUTPUT);
      pinMode(cfg::pins::SRStrobe, OUTPUT);

      pinMode(cfg::pins::leftMotorPWM, OUTPUT);
      pinMode(cfg::pins::rightMotorPWM, OUTPUT);

      pinMode(cfg::pins::nrfInterrupt, INPUT);
      pinMode(cfg::pins::nrfSS, OUTPUT);

      pinMode(cfg::pins::armUnoSS, OUTPUT);
      digitalWrite(cfg::pins::armUnoSS, HIGH);
      pinMode(cfg::pins::SPIMISO, INPUT);
      pinMode(cfg::pins::SPIMOSI, OUTPUT);
      pinMode(cfg::pins::SPIClock, OUTPUT);

    //START SERIAL CONNECTION
      #ifdef DEBUG_SERIAL
        Serial.begin(cfg::k_serialBaudRate);
        Serial.println("Serial connected.");
      #endif

    //SETUP SR
      sr.forcePush(0);

    //SETUP MOTOR DRIVER
      driver.drive(0, 0);
      #ifdef DEBUG_MOTOR
        driver.drive(250, 250);
        delay(1000);
        driver.drive(-250, -250);
        delay(1000);
        driver.drive(0, 250);
        delay(500);
        driver.drive(250, 0);
        delay(500);
        driver.drive(0, 0);
      #endif      
    
    //SETUP & CALIBRATE QTR
      #ifdef DEBUG_SERIAL
        Serial.println("QTR calibrating...");
      #endif
      qtr.setTypeAnalog();
      qtr.setEmitterPin(cfg::pins::emitter);
      qtr.setSensorPins(cfg::pins::qtr, cfg::k_sensorCount);
        /* BACKUP
        for(uint16_t i = 0; i < 400; i++) {
          qtr.calibrate(cfg::k_readMode);
        }
        */
      for(uint8_t i = 0; i < cfg::k_calibrationWiggleCount; i++) {
        lastTime = millis();
        driver.drive(-cfg::k_calibrationMoveSpeed, cfg::k_calibrationMoveSpeed);
        while((millis() - lastTime) < cfg::k_calibrationMoveDuration / 2.f) {
          qtr.calibrate(cfg::k_readMode);
        }
        lastTime = millis();
        driver.drive(cfg::k_calibrationMoveSpeed, -cfg::k_calibrationMoveSpeed);
        while(millis() - lastTime < cfg::k_calibrationMoveDuration) {
          qtr.calibrate(cfg::k_readMode);
        }
        lastTime = millis();
        driver.drive(-cfg::k_calibrationMoveSpeed, cfg::k_calibrationMoveSpeed);
        while((millis() - lastTime) < cfg::k_calibrationMoveDuration / 2) {
          qtr.calibrate(cfg::k_readMode);
        }
        driver.drive(0, 0);
      }
      #ifdef DEBUG_SERIAL
        Serial.println("QTR calibrated.");
      #endif

    //SETUP DEBUG LEDS
      /*leds.redOff();
      leds.greenOff();
      leds.blueOff();*/
      #ifdef DEBUG_LED
        leds.redOn();
        delay(333);
        leds.redOff();
        leds.greenOn();
        delay(333);
        leds.greenOff();
        leds.blueOn();
        delay(333);
        leds.blueOff();  
      #endif    

  //GET COMMANDS
    numberOfCommands = GetCommandsWithNRF(&commands);

  //SETUP SPI 
    spi.enable();
}
int16_t leftSpeed = 1;
int16_t rightSpeed = 1;
void loop() {
  //TIME
    lastTime = currentTime;
    currentTime = micros();
    dT = currentTime - lastTime;

  //POSITION & LINE COLOR  
    position = (lineColorFlag) ? qtr.readLineWhite(sensorValues, cfg::k_readMode) : qtr.readLineBlack(sensorValues, cfg::k_readMode);
    lastLineColorFlag = lineColorFlag;
    /*if(sensorValues[0] < 300 && sensorValues[1] < 700 && (sensorValues[2] > 300 || sensorValues[3] > 300) && sensorValues[4] < 700 && sensorValues[5] < 300) {
      if(lastLineColorFlag) {
        lineColorFlag = 0;
        position = qtr.readLineBlack(sensorValues, cfg::k_readMode);
      }
    }
    else if(sensorValues[0] > 700 && sensorValues[1] > 300 && (sensorValues[2] < 700 || sensorValues[3] < 700) && sensorValues[4] > 300 && sensorValues[5] > 700) {
      if(!lastLineColorFlag) {
        lineColorFlag = 1;
        position = qtr.readLineWhite(sensorValues, cfg::k_readMode);
      }
    }*/
    if(sensorValues[0] < 300 && ((sensorValues[1] < 300 && sensorValues[2] < 300 && sensorValues[3] > 700 && sensorValues[4] > 700) || 
                                 (sensorValues[1] > 700 && sensorValues[2] > 700 && sensorValues[3] < 300 && sensorValues[4] < 300) || 
                                 (sensorValues[1] < 300 && sensorValues[2] > 700 && sensorValues[3] > 700 && sensorValues[4] < 300 )) && sensorValues[5] < 300) {
      if(lastLineColorFlag) {
        lineColorFlag = 0;
        position = qtr.readLineBlack(sensorValues, cfg::k_readMode);
      }
    }
    else if(sensorValues[0] > 700 && ((sensorValues[1] > 700 && sensorValues[2] > 700 && sensorValues[3] < 300 && sensorValues[4] < 300) || 
                                      (sensorValues[1] < 300 && sensorValues[2] < 300 && sensorValues[3] > 700 && sensorValues[4] > 700) || 
                                      (sensorValues[1] > 700 && sensorValues[2] < 300 && sensorValues[3] < 300 && sensorValues[4] > 700)) && sensorValues[5] > 700) {
      if(!lastLineColorFlag) {
        lineColorFlag = 1;
        position = qtr.readLineWhite(sensorValues, cfg::k_readMode);
      }
    }
    if(lineColorFlag) {
      for(uint8_t i = 0; i < cfg::k_sensorCount; i++) {
        sensorValues[i] = 1000 - sensorValues[i];
      }
    }
    #ifdef DEBUG_LED
      if(lastLineColorFlag != lineColorFlag) {
        leds.redToggle();
        lineColorTime = micros() - 1;           
      }
    #endif
    #ifdef DEBUG_SERIAL
      sprintf(serialPrintBuffer, "Sensor Values: %" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\n", sensorValues[0], sensorValues[1], sensorValues[2],
                                                                                                                                  sensorValues[3], sensorValues[4], sensorValues[5]);
      Serial.print(serialPrintBuffer);
    #endif    

  //ERROR & IS ON LINE
    lastError = currentError;
    currentError = position - ((cfg::k_sensorCount - 1) * 500);
    lastOnLineFlag = onLineFlag;
    if((sensorValues[2] > 500 && sensorValues[3] > 300) || (sensorValues[2] > 300 && sensorValues[3] > 500)) {
      onLineFlag = 1;
      //leds.greenOn();
    }
    else {
      onLineFlag = 0;
      //leds.greenOff();
    }

  //STATE MACHINE
    #ifdef LOGI_TEST
      /*if(!runOnceFlag) {
        driver.drive(cfg::k_forwardSpeed, cfg::k_forwardSpeed);
        delay(cfg::k_forwardDuration);
        runOnceFlag = 1;
      }
      driver.drive(cfg::k_turnRightSpeed, -cfg::k_turnRightSpeed);
      if(!lastOnLineFlag && onLineFlag) {
        runOnceFlag = 0;
      }*/
      #ifdef DEBUG_SERIAL
        leftSpeed = cfg::k_base + PIDSpeedModifier;
        rightSpeed = cfg::k_base - PIDSpeedModifier;
        sprintf(serialPrintBuffer, "                                                                 Left: %" PRId16 "\n", leftSpeed);
        Serial.print(serialPrintBuffer);
        sprintf(serialPrintBuffer, "                                                                                                      Right: %" PRId16 "\n", rightSpeed);
        Serial.print(serialPrintBuffer);
        sprintf(serialPrintBuffer, "                                                                                                                          Error: %" PRId16 "\n", currentError);
        Serial.print(serialPrintBuffer);
        sprintf(serialPrintBuffer, "Sensor Values: %" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\n", sensorValues[0], sensorValues[1], sensorValues[2],
                                                                                                                            sensorValues[3], sensorValues[4], sensorValues[5]);
        Serial.print(serialPrintBuffer);
      #endif
    #else
    

    
    switch (commands[commandCounter]) {                   
      case 'f': //FOLLOW LINE UNTIL SENSOR REACHES A CROSSROAD
        if((sensorValues[2] > 500 && sensorValues[3] > 500) && ((sensorValues[0] > 700 && sensorValues[1] > 700) || (sensorValues[4] > 700 && sensorValues[5] > 700))) {
          if(!junctionDoubleTestFlag) {
            junctionDoubleTestFlag = 1;
            driver.drive(cfg::k_forwardSpeed, cfg::k_forwardSpeed);
            delay(50);
          }
          else {
            if(commands[commandCounter + 1] == 'f') {
              driver.drive(cfg::k_forwardSpeed, cfg::k_forwardSpeed);
              delay(cfg::k_forwardDuration / 2);              
            }
            else {
              driver.drive(-200, -200);
              delay(35);
              driver.drive(0, 0);              
            }
            junctionDoubleTestFlag = 0;
            commandCounter++;
            #ifdef DEBUG_LED
              leds.blueToggle();            
            #endif
          }
        } 
        else {
          junctionDoubleTestFlag = 0;
          PIDSpeedModifier = cfg::k_p * currentError + cfg::k_d * (currentError - lastError) / dT;
          driver.drive(cfg::k_base + PIDSpeedModifier, cfg::k_base - PIDSpeedModifier);
        }
        #ifdef DEBUG_SERIAL
          leftSpeed = cfg::k_base + PIDSpeedModifier;
          rightSpeed = cfg::k_base - PIDSpeedModifier;
          sprintf(serialPrintBuffer, "                                                                 Left: %" PRId16 "\n", leftSpeed);
          Serial.print(serialPrintBuffer);
          sprintf(serialPrintBuffer, "                                                                                                      Right: %" PRId16 "\n", rightSpeed);
          Serial.print(serialPrintBuffer);
          sprintf(serialPrintBuffer, "                                                                                                                          Error: %" PRId16 "\n", currentError);
          Serial.print(serialPrintBuffer);
          sprintf(serialPrintBuffer, "Sensor Values: %" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\n", sensorValues[0], sensorValues[1], sensorValues[2],
                                                                                                                              sensorValues[3], sensorValues[4], sensorValues[5]);
          Serial.print(serialPrintBuffer);
        #endif
        break;
      case 'r': //GO FORWARD FOR A WHILE THEN TURN RIGHT UNTIL SENSOR'S ON LINE AGAIN
        if(!runOnceFlag) {
          driver.drive(cfg::k_forwardSpeed, cfg::k_forwardSpeed);
          delay(cfg::k_forwardDuration);
          runOnceFlag = 1;
        }
        driver.drive(cfg::k_turnRightSpeed, -cfg::k_turnRightSpeed);
        if((!lastOnLineFlag) && onLineFlag) {
          runOnceFlag = 0;
          commandCounter++;
        }
        break;
      case 'l': //GO FORWARD FOR A WHILE THEN TURN LEFT UNTIL SENSOR'S ON LINE AGAIN
        if(!runOnceFlag) {
          driver.drive(cfg::k_forwardSpeed, cfg::k_forwardSpeed);
          delay(cfg::k_forwardDuration);
          runOnceFlag = 1;
        }
        driver.drive(-cfg::k_turnLeftSpeed, cfg::k_turnLeftSpeed);
        if((!lastOnLineFlag) && onLineFlag) {
          runOnceFlag = 0;
          commandCounter++;
        }
        break;
      case 's':
        driver.drive(0, 0);
        delay(5000);
        commandCounter++;
        break; 
      case 'e':

          if(!eHasArrived){
            timeOfArrivalOfE=millis();
            eHasArrived=1;
          }

          if((millis()-timeOfArrivalOfE)>900){
          PIDSpeedModifier = cfg::k_p * currentError + cfg::k_d * (currentError - lastError) / dT;
          driver.drive(cfg::k_base + PIDSpeedModifier, cfg::k_base - PIDSpeedModifier);
          }
          
          break;
          
        
    }
    #endif
}
