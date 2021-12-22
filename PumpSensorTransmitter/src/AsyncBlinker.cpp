//
// Created by UlrikKarlsson on 2021-12-08.
//

#include "AsyncBlinker.h"

AsyncBlinker::AsyncBlinker(int pin, int onTime, int offTime) {
  m_timeAtLastRun = millis();
  m_timeSinceLastCycle = 0;
  m_pin = pin;
  setOnTime(onTime);
  setOffTime(offTime);
}

AsyncBlinker::~AsyncBlinker() {}

void AsyncBlinker::setOnTime(int onTime) { m_onTime = onTime; }

void AsyncBlinker::setOffTime(int offTime) { m_offTime = offTime; }

void AsyncBlinker::run(unsigned long currentTime) {
  if (m_numberOfBlinks > 0) {
    m_timeSinceLastCycle += currentTime - m_timeAtLastRun;
    if (m_timeSinceLastCycle < m_onTime) {
      digitalWrite(m_pin, 1);
    } else if (m_timeSinceLastCycle < m_onTime + m_offTime) {
      digitalWrite(m_pin, 0);
    } else {
      m_numberOfBlinks -= 1;
      m_timeSinceLastCycle = 0;
    }
    m_timeAtLastRun = currentTime;
  }
}

void AsyncBlinker::blink(int numberOfBlinks) {
  m_numberOfBlinks = numberOfBlinks;
}

void AsyncBlinker::resetTimer() {
  m_timeSinceLastCycle = 0;
  m_timeAtLastRun = millis();
}
