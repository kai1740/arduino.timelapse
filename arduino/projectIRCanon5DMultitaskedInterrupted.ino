#include "Device.h"
#include "PCInterrupt.h"
#include "irController.h"
#include "myHardware.h"

#define irLED 12
#define cLED 13
#define gLED 11
#define servoPIN 3
#define servoPOW 4
#define irREC 2




int key;

IRController controller;

void setup() {
  Serial.begin(9600);
  pinMode(irLED, OUTPUT); 
  pinMode(gLED, OUTPUT);
  pinMode(cLED, OUTPUT);
  pinMode(servoPIN, OUTPUT);
  pinMode(servoPOW, OUTPUT); 
  digitalWrite(servoPOW, HIGH);
  pinMode(irREC, INPUT);
  
  int res = controller.begin(irREC, OTHER_DEVICE);
  if (res != SUCCESS) {
    Serial.println("IRController error"); 
    Serial.println(res);
  }
}

boolean isTimelapseEnabled = false;
boolean isTimelapseOpositeDirection = false;
int menuState = 0;

unsigned int EXPOSURE_DURATION = 2000;
unsigned int delayAfterExposure = 200;
unsigned int SERVO_DURATION = 200; //step 200
unsigned int delayAfterServo = 200;

unsigned int SERVO_SPEED = 15; //step 15
unsigned int SERVO_DURATION_LOOPS = SERVO_DURATION/20-1;

// multitasking
long waitUntil11 = 0;
long waitUntil12 = 0;
long waitUntil13 = 0;
long pauseFlashDelay = 1000;
long timestamp = 0;
int step = 1;
boolean ledState = false;

long mark1 = 0;

void loop() {
  //look for a header pulse from the IR Receiver
  timestamp = millis();

  //Serial.print(".");
   
  if(isTimelapseEnabled){
     
     if (step==1 && timestamp >= waitUntil11) { // multitasking
        sendInfraredSignal();
        step=2;
        waitUntil12 = timestamp + EXPOSURE_DURATION + delayAfterExposure;
     } 

     if (step==2 && timestamp >= waitUntil12) {
       isTimelapseOpositeDirection ? servoRight() : servoLeft();
       step=1; 
       waitUntil11 = timestamp + SERVO_DURATION + delayAfterServo;
     }
  } else {
    
    // Blinking
    if(menuState != 0 && timestamp >= waitUntil13){
      waitUntil13 = timestamp + pauseFlashDelay;
      
      if(menuState==1){
        for(int i = 0; i < EXPOSURE_DURATION/1000; i++){
          digitalWrite(cLED, HIGH);
          delay(50);
          digitalWrite(cLED, LOW);
          delay(50);
        }
      } else if(menuState==2){
        for(int i = 0; i < SERVO_DURATION/200; i++){
          digitalWrite(gLED, HIGH);
          delay(50);
          digitalWrite(gLED, LOW);
          delay(50);
        }
      } else if(menuState==3){
        for(int i = 0; i < SERVO_SPEED/15; i++){
          digitalWrite(cLED, HIGH);
          digitalWrite(gLED, HIGH);
          delay(50);
          digitalWrite(cLED, LOW);
          digitalWrite(gLED, LOW);
          delay(50);
        }
      }
    }
    
    if(menuState==0){
      digitalWrite(cLED, HIGH);
    }
    
  }
  
  
  key = controller.read();

  if (key >= 0) 
  {
    switch(key)
    {
      case 17121: 
        //Serial.println(" Up");
        if(!isTimelapseEnabled){
          if(menuState==1){
            if(EXPOSURE_DURATION<10000) EXPOSURE_DURATION += 1000;
          } else if(menuState==2){
            if(SERVO_DURATION<1800) SERVO_DURATION += 200;
            SERVO_DURATION_LOOPS = SERVO_DURATION/20-1;
          } else if(menuState==3){
            if(SERVO_SPEED<100) SERVO_SPEED += 15;
          }
        }
        break;
      case 17249: 
        //Serial.println(" Down"); 
        if(!isTimelapseEnabled){
          if(menuState==1){
            if(EXPOSURE_DURATION>1000) EXPOSURE_DURATION -= 1000;
          } else if(menuState==2){
            if(SERVO_DURATION>200) SERVO_DURATION -= 200;
            SERVO_DURATION_LOOPS = SERVO_DURATION/20-1;
          } else if(menuState==3){
            if(SERVO_SPEED>15) SERVO_SPEED -= 15;
          }
        }
        break;
      case 16865: 
          //Serial.println(" Right"); 
          if(isTimelapseEnabled) isTimelapseOpositeDirection = true;
          else servoRight(); 
          break;
      case 16929: 
          //Serial.println(" Left"); 
          if(isTimelapseEnabled) isTimelapseOpositeDirection = false;
          else servoLeft(); 
          break;
      case 16673: toggleTimelapse(); break;
      case 16545: 
          //Serial.println(" Menu"); 
          //sendInfraredSignal(); 
          toggleMenu();
          break;
      default: Serial.println(key);
    }
  }

  delay(100);   
}
//-------------------------------
void toggleMenu() {
   if(menuState<3)menuState++;
   else menuState=0;
   
   switch(menuState)
    {
      case 1: Serial.println("Exposure"); break;
      case 2: Serial.println("ServoDuration");break;
      case 3: Serial.println("ServoSpeed"); break;
      default: Serial.println("Idle"); 
    }
  
}
void toggleTimelapse() {
  digitalWrite(gLED, HIGH);
  delay(50);
  digitalWrite(gLED, LOW);
  isTimelapseEnabled ? Serial.println("Stoped") : Serial.println("Started"); 
  isTimelapseEnabled = ! isTimelapseEnabled;
}
//-------------------------------
void servoStop() {
   servoGo(1500,5);
}
void servoLeft() {
  Serial.println("GO LEFT");
  servoGo(1500+SERVO_SPEED,SERVO_DURATION_LOOPS);
}
void servoRight() {
  Serial.println("GO RIGHT");
  servoGo(1500-SERVO_SPEED,SERVO_DURATION_LOOPS);
}
void servoGo(int _speed,int _duration) {
  digitalWrite(cLED, HIGH);
  mark1 = millis();
  for(int i = 0; i < _duration; i++) { 
    digitalWrite(servoPIN,HIGH);
    delayMicroseconds(_speed); //1500 is center, 1300 < 1700 range 
    digitalWrite(servoPIN,LOW);
    delay(20);  //20 ms
 }
 /*Serial.println();
 Serial.print(_speed);
 Serial.print(":");
 Serial.print(_duration);
 Serial.print(":");
 Serial.print(millis()-mark1);
 Serial.println();*/
 
 digitalWrite(cLED, LOW); 
}
//-------------------------------
unsigned int pulseDuration = 11;
void sendInfraredSignal(){
  
  Serial.println("SENDING IR");
   
  digitalWrite(cLED, HIGH);
  
  for(int i = 0; i < 16; i++) { 
    digitalWrite(irLED, HIGH);
    delayMicroseconds(pulseDuration);
    digitalWrite(irLED, LOW);
    delayMicroseconds(pulseDuration);
   } 
   delayMicroseconds(7330); 
   for(int i = 0; i < 16; i++) { 
     digitalWrite(irLED, HIGH);
     delayMicroseconds(pulseDuration);
     digitalWrite(irLED, LOW);
     delayMicroseconds(pulseDuration);
   }
   
    digitalWrite(cLED, LOW);
}
