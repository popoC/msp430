#include "TinyGPS++.h"
#include "Timer.h"

TinyGPSPlus gps;
TinyGPSCustom GPRMC_time(gps, "GPRMC", 1); 
int outtime = 10;
int now_s;
int now_m;
int now_h;
int now_d;


int ledPin_s = 12;                  // LED test pin
int ledPin_m = 11;                  // LED test pin
int ledPin_h = 10;                  // LED test pin
int ledPin_d = 9;                   // LED test pin

int ppsPin = 2;                   // LED test pin
int ppscome = 0;
//RISING

Timer t;

void setup() {
  // put your setup code here, to run once:
   pinMode(ledPin_s, OUTPUT);       // Initialize LED pin
   pinMode(ledPin_m, OUTPUT);       // Initialize LED pin
   pinMode(ledPin_h, OUTPUT);       // Initialize LED pin
   pinMode(ledPin_d, OUTPUT);       // Initialize LED pin

   
   pinMode(ppsPin, INPUT_PULLUP);
   
   Serial1.begin(9600); 
   Serial.begin(9600);
   
   attachInterrupt(digitalPinToInterrupt(ppsPin),ppsinISR,RISING);
  // interrupts();
}

void loop() {
  // put your main code here, to run repeatedly:
   t.update();
   if( Serial1.available()>0){
      gps.encode(Serial1.read());
      if(gps.time.isUpdated() && GPRMC_time.isUpdated()){

        now_s =  gps.time.second(); // 0-59
        now_m =  gps.time.minute(); // 0-59
        now_h =  gps.time.hour(); // 0-24
        now_d =  gps.date.day();  // 1-31
        if((now_s % outtime)==(outtime-1)){
              Serial.println(GPRMC_time.value() );
              Serial.println(now_m);
          
             ppscome = 1;
        }
           
      }
   }

   
}

void ppsinISR(){
    if(ppscome == 1){ 
     digitalWrite(ledPin_s, HIGH);
     t.after(100, closeP_s); //  ms
       ppscome = 0;
    }
    digitalWrite(ledPin_m, HIGH);
    digitalWrite(ledPin_h, HIGH);
    digitalWrite(ledPin_d, HIGH);
     t.after(10 * now_m, closeP_m);   
     t.after(10 * now_h, closeP_h);   
     t.after(10 * now_d, closeP_d);   
}


void closeP_s(){
  digitalWrite(ledPin_s, LOW);
}
void closeP_m(){
  digitalWrite(ledPin_m, LOW);
}
void closeP_h(){
  digitalWrite(ledPin_h, LOW);
}
void closeP_d(){
  digitalWrite(ledPin_d, LOW);
}

