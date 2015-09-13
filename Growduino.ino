/*
????????
- relayState in EEPROM
????????
 */
 
#define REG_SELECT 9
#define IRQ_PIN 2
#define SDA_PIN 7
#define SCL_PIN 8
#define EMAX 10
#define WATERING_TIME 1000000

//------- UART COMMANDS -------
#define REL_ON      '1'
#define REL_OFF     '0'
#define PRINT_TIME  '2'
#define SET_TIME    '3'
//-----------------------------
 
//#include <Wire.h>
#include <Time.h>
#include <DS1307.h>
//#include <DS1307RTC.h>
#include <ASCII.h>
#include <NokiaLCD.h>

#include <SPI.h>

//-- relay pins --
//#define relNum 4
//#define rel1Pin 6
//#define rel2Pin 3
//#define rel3Pin 4
//#define rel4Pin 5
//----------------
//int relPin[relNum] = {rel1Pin, rel2Pin, rel3Pin, rel4Pin};

//-- rel shed --
int hourOn = 18;
int minOn = 11;
int hourOff = 19;
int minOff = 0;
//---------------

//--------------- EVENTS struct & class ---------------
enum EType {ON, OFF, WATERING};

struct Event {
  Time time;
  int relNum;
  EType type;
};
// -- CLASS --
class EList {
  private:
          Event event[EMAX];
          int curEvent;
          int numEvent;
  
  public:         
          EList();
          Event getEvent(int);
          int addEvent(Time, int, EType);
          //int addEvent(Event);
          //int showEvent(int); // DEBUG   
          int eventProc(Time);  // DEBUG    
};
// ---------

EList :: EList(){    // constructor
   curEvent = 0;
   numEvent = 0; 
}

Event EList :: getEvent(int num){
    return event[num];
}

int EList :: addEvent( Time time, int relNum, EType type){
    if (numEvent >= EMAX) return 1;
    event[numEvent].time = time;
    event[numEvent].relNum = relNum;
    event[numEvent].type = type;
    return 0;   
}

/*int EList :: addEvent( Event e){
    if (numEvent >= EMAX) return 1;
    event[numEvent] = e;
    return 0;   
}*/

/*int EList :: showEvent(int n){    //DEBUG
    if (!Serial) return 1;
    Serial.print(event[n].time.hour);
    Serial.print(':');
    Serial.print(event[n].time.min);
    Serial.print(':');
    Serial.println(event[n].time.sec);
    return 0;
}*/
    
int EList :: eventProc(Time t){ // DEBUG
    for (int i = 0; i < EMAX; i ++){
       if (event[i].time.hour == t.hour and event[i].time.min == t.min and t.sec < 5){
          switch (event[i].type){
             case ON:
                relOn(event[i].relNum);
                return 0;
             case OFF:
                relOff(event[i].relNum);
                return 0;
             case WATERING:
                watering(event[i].relNum);
                return 0;
             default: 
                return 1;
          }
       } 
    }
}
//---------------------------------------------------------

DS1307 rtc (SDA_PIN, SCL_PIN);
Time curTime;

byte relState = 0;
EList eList;

Event tmpE;

// the setup function runs once when you press reset or power the board
void setup() {
  
  Time t;
  t.hour = 19;
  t.min = 47;
  t.sec = 0;
  eList.addEvent(t, 0, WATERING);
  
  SPI.begin();
  pinMode(REG_SELECT, OUTPUT);
  digitalWrite(REG_SELECT, LOW);
  SPI.transfer(0);
  digitalWrite(REG_SELECT, HIGH);
  
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);
  curTime = rtc.getTime();
  //delay(10);
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  attachInterrupt(0, irq0, RISING);
  interrupts();
}

//--------------------- LOOP ----------------------------
void loop() {  
  byte cmd;
  if(Serial.available() > 0){
    cmd = Serial.read();
    cmdProc(cmd);
  }
  delay(500);
}
//------------------------IRQ-----------------------------
 void irq0(){
   noInterrupts();
   curTime = rtc.getTime();
   eList.eventProc(curTime);
   interrupts();
 }
//---------------------------------------------------------

 void cmdProc(byte cmd){
   Event e;
   switch (cmd){
      case REL_ON:
        while(Serial.available() < 1){}
        cmd = Serial.read();
        relOn(cmd - 48);  // DEBUG
        break;
        
      case REL_OFF:
        while(Serial.available() < 1){}
        cmd = Serial.read();
        relOff(cmd - 48);    // DEBUG
        break;
         
      case PRINT_TIME:
         Serial.println(rtc.getTimeStr(FORMAT_LONG));
         break;
         
      case SET_TIME:
        setRTCtime(18, 13, 0);
        break;
        
      case '4':
        //eList.showEvent(0);
        e = eList.getEvent(0);
        Serial.print(e.time.hour);
        Serial.print(':');
        Serial.print(e.time.min);
        Serial.print(':');
        Serial.println(e.time.sec);
        break;
        
      case '5':
        relOn(2);
        break;
        
      case '6':
        relOff(2);
        break;
         
      default: 
         Serial.println("Command error");
   }
 }

/* int eventProc (EType te, int relNum){
   switch type{
     case ON:
       relOn(relNum);
       break;
     case OFF:
       relOff(relNum);
       break;
     case WATERING:
     
       break;
   }
   return 0;
 }*/

 void relOn (int num){
    bitSet(relState, num);
    digitalWrite(REG_SELECT, LOW);
    SPI.transfer(relState);
    digitalWrite(REG_SELECT, HIGH);
 }
 
 void relOff (int num){
    bitClear(relState, num);
    digitalWrite(REG_SELECT, LOW);
    SPI.transfer(relState);
    digitalWrite(REG_SELECT, HIGH);
 }
 
 void watering (int num){
     relOn(num);
     delay(WATERING_TIME);
     relOff(num);
 }
 
 int setRTCtime(int h, int m, int s){
   rtc.setTime(h, m, s); 
   return 0; 
 }
