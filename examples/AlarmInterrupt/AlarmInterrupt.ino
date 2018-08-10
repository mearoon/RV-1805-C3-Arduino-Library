/*
  RV-1805-C3 Alarm Interrupt Example
  
  Sets an alarm based on calendar date and time, and triggers an interrupt.

  Copyright (c) 2018 Macro Yau

  https://github.com/MacroYau/RV-1805-C3-Arduino-Library
*/

#include <RV1805C3.h>

const unsigned int interruptPin = 2; // Connected to pin 3 of RV-1805-C3

RV1805C3 rtc;

bool interruptTriggered = false;

void setup() {
  Serial.begin(9600);
  Serial.println("RV-1805-C3 Alarm Interrupt Example");
  Serial.println();

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), alarm, FALLING);

  Wire.begin();

  while (rtc.begin() == false) {
    Serial.println("Failed to detect RV-1805-C3!");
    delay(5000);
  }

  rtc.reset(); // Clears previous states
  delay(50); // 50 ms delay to ensure the RTC resets properly

  rtc.setDateTimeFromISO8601("2018-01-01T08:00:00"); // Hard-coded date time to demonstrate alarm function
  rtc.synchronize(); // Writes the new date time to RTC

  Serial.print("Start: ");
  Serial.println(rtc.getCurrentDateTime());

  rtc.enableInterrupt(INTERRUPT_ALARM);

  rtc.setAlarmFromISO8601("2018-01-01T08:00:10"); // Alarm will go off at this exact moment (10 seconds after reset)
  rtc.setAlarmMode(ALARM_ONCE_PER_DAY); // Daily alarm

  rtc.sleep();
}

void loop() {
  if (interruptTriggered) {
    Serial.print("Alarm: ");
    Serial.println(rtc.getCurrentDateTime());
    rtc.clearInterrupts();
    interruptTriggered = false;
  }
}

void alarm() {
  interruptTriggered = true;
}
