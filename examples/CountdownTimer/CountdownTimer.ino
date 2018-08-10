/*
  RV-1805-C3 Countdown Timer Example
  
  Sets a repeating countdown timer that triggers an interrupt.

  Copyright (c) 2018 Macro Yau

  https://github.com/MacroYau/RV-1805-C3-Arduino-Library
*/

#include <RV1805C3.h>

const unsigned int interruptPin = 2; // Connected to pin 3 of RV-1805-C3

RV1805C3 rtc;

bool interruptTriggered = false;

void setup() {
  Serial.begin(9600);
  Serial.println("RV-1805-C3 Countdown Timer Example");
  Serial.println();

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), timerExpired, FALLING);

  Wire.begin();

  while (rtc.begin() == false) {
    Serial.println("Failed to detect RV-1805-C3!");
    delay(5000);
  }

  rtc.clearInterrupts(); // Clears previous states

  rtc.enableInterrupt(INTERRUPT_TIMER);

  Serial.print("Countdown Timer Started: ");
  Serial.println(rtc.getCurrentDateTime());
  
  rtc.setCountdownTimer(5, COUNTDOWN_SECONDS); // 5-second countdown timer

  rtc.sleep();
}

void loop() {
  if (interruptTriggered) {
    Serial.print("Countdown Timer Expired: ");
    Serial.println(rtc.getCurrentDateTime());
    rtc.clearInterrupts();
    interruptTriggered = false;
  }
}

void timerExpired() {
  timerTriggered = true;
}
