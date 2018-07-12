#include <Wire.h>
#include <DS1307.h>
/*
Dylan Rush 2018
http://blog.dylanhrush.com/2018/07/nixie-tube-clock.html

Based off of the tutorial for the DS1307 module I was using:

https://www.dfrobot.com/wiki/index.php/Real_Time_Clock_Module_(DS1307)_V1.1_(SKU:DFR0151)


example: 14,03,25,02,13,55,10   2014.03.25 tuesday 13:55:10
*/
String comdata = "";
int mark = 0;
//store the current time data
int rtc[7];
//store the set time data
byte rr[7];
//light pin
int ledPin = 13;
//initial light

int buttonState = LOW;
int pressedDuration = 0;
int loopDelayMsec = 10;
int longPressThresholdMsec = 1000;

void setup() {
  DDRC |= _BV(2) | _BV(3); // POWER:Vcc Gnd
  PORTC |= _BV(3); // VCC PINC3
  pinMode(ledPin, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A3, HIGH);
  digitalWrite(A2, LOW);
  //initial baudrate
  Serial.begin(9600);
  Serial.write("hello");
  //get current time
  RTC.get(rtc, true);
  //if time is wrong reset to default time
  Serial.println(rtc[6]);
  if (rtc[6] < 2018) {
    //stop rtc time
    RTC.stop();
    RTC.set(DS1307_SEC, 1);
    RTC.set(DS1307_MIN, 55);
    RTC.set(DS1307_HR, 11);
    RTC.set(DS1307_DOW, 7);
    RTC.set(DS1307_DATE, 12);
    RTC.set(DS1307_MTH, 2);
    RTC.set(DS1307_YR, 18);
    //start rtc time
    RTC.start();
  }

  RTC.SetOutput(DS1307_SQW32KHZ);
  pinMode(2, OUTPUT);
  pinMode(A7, INPUT);
}

void loop() {
  int i;
  //get current time 
  RTC.get(rtc, true);

  int seconds_ones = rtc[0] % 10;
  int seconds_tens = rtc[0] / 10;
  int minutes_ones = rtc[1] % 10;
  int minutes_tens = rtc[1] / 10;
  int hours_ones = rtc[2] % 10;
  int hours_tens = rtc[2] / 10;

  digitalWrite(2, hours_tens & 0x01);

  nixieWrite(3, 4, 5, 6, hours_ones);

  nixieWrite(7, 8, 9, 10, minutes_tens);
  // miswire
  if (minutes_ones == 5) {
    minutes_ones = 1;
  } else if (minutes_ones == 1) {
    minutes_ones = 5;
  }
  nixieWrite(11, 12, A0, A1, minutes_ones);

  int newButtonState = analogRead(A7) < 500 ? LOW : HIGH;

  if (newButtonState == HIGH && buttonState == LOW) {
    Serial.println("press");
    // Just started pressing
    buttonState = HIGH;
  } else if (newButtonState == HIGH && buttonState == HIGH) {
    pressedDuration += loopDelayMsec;
  } else if (newButtonState == LOW && buttonState == HIGH) {
    // Just released
    Serial.println("Released");

    buttonState = LOW;
    int newHours = rtc[2];
    int newMinutes = rtc[1];
    if (pressedDuration > longPressThresholdMsec) {
      newHours = (newHours + 1) % 12;
      if (newHours == 0) {
        newHours = 12;
      }
    } else {
      newMinutes = (newMinutes + 1) % 60;
    }
    pressedDuration = 0;
    RTC.stop();
    RTC.set(DS1307_MIN, newMinutes);
    RTC.set(DS1307_HR, newHours);
    RTC.start();
  } else {
    buttonState = newButtonState;
  }

  delay(loopDelayMsec);

  int j = 0;
  //read all the data
  while (Serial.available() > 0) {
    comdata += char(Serial.read());
    delay(2);
    mark = 1;
  }
  //if data is all collected,then parse it
  if (mark == 1) {
    Serial.println(comdata);
    Serial.println(comdata.length());
    //parse data
    for (int i = 0; i < comdata.length(); i++) {
      //if the byte is ',' jump it,and parse next value
      if (comdata[i] == ',') {
        j++;
      } else {
        rr[j] = rr[j] * 10 + (comdata[i] - '0');
      }
    }
    comdata = String("");
    RTC.stop();
    RTC.set(DS1307_SEC, rr[6]);
    RTC.set(DS1307_MIN, rr[5]);
    RTC.set(DS1307_HR, rr[4]);
    RTC.set(DS1307_DOW, rr[3]);
    RTC.set(DS1307_DATE, rr[2]);
    RTC.set(DS1307_MTH, rr[1]);
    RTC.set(DS1307_YR, rr[0]);
    RTC.start();
    mark = 0;
  }
}

void nixieWrite(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t value) {
  //D is most significant bit
  //A is least significant bit
  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);
  pinMode(d, OUTPUT);
  digitalWrite(d, (value & 0x08) >> 3);
  digitalWrite(c, (value & 0x04) >> 2);
  digitalWrite(b, (value & 0x02) >> 1);
  digitalWrite(a, value & 0x01);
}

