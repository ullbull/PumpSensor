//
// Created by UlrikKarlsson on 2021-12-08.
//

#ifndef TEST_ASYNCBLINKER_H
#define TEST_ASYNCBLINKER_H

class AsyncBlinker {
private:
  int m_pin;
  int m_delay;
  int m_onTime;
  int m_offTime;
  unsigned long m_timeAtLastRun;
  unsigned long m_timeSinceLastCycle;
  int m_numberOfBlinks = 0;

public:
  AsyncBlinker(int pin, int delay, int offTime, int onTime);

  ~AsyncBlinker();

  void setDelay(int delay);

  void setOnTime(int onTime);

  void setOffTime(int offTime);

  // This method must be called on every round in main loop
  void run(unsigned long currentTime);

  // Call this method when you want to blink the LED
  void blink(int numberOfBlinks);

  void resetTimer();
};

#endif // TEST_ASYNCBLINKER_H

//
// Created by UlrikKarlsson on 2021-12-08.
//

AsyncBlinker::AsyncBlinker(int pin, int delay, int onTime, int offTime) {
  m_timeAtLastRun = millis();
  m_timeSinceLastCycle = 0;
  m_pin = pin;
  setDelay(delay);
  setOnTime(onTime);
  setOffTime(offTime);
}

AsyncBlinker::~AsyncBlinker() {}

void AsyncBlinker::setDelay(int delay) { m_delay = delay; }

void AsyncBlinker::setOnTime(int onTime) { m_onTime = onTime; }

void AsyncBlinker::setOffTime(int offTime) { m_offTime = offTime; }

void AsyncBlinker::run(unsigned long currentTime) {
  if (m_numberOfBlinks > 0) {
    m_timeSinceLastCycle += currentTime - m_timeAtLastRun;
    if (m_timeSinceLastCycle < m_delay) {
      digitalWrite(m_pin, 0);
    } else if (m_timeSinceLastCycle < m_delay + m_onTime) {
      digitalWrite(m_pin, 1);
    } else if (m_timeSinceLastCycle < m_delay + m_onTime + m_offTime) {
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
  resetTimer();
}

void AsyncBlinker::resetTimer() {
  m_timeSinceLastCycle = 0;
  m_timeAtLastRun = millis();
}

int LED_RED = 4;
int LED_GREEN = 3;
int LED_BLUE = 2;
int button1Pin = 5;

AsyncBlinker blinkLedRed = AsyncBlinker(LED_RED, 0, 400, 600);
AsyncBlinker blinkLedGreen = AsyncBlinker(LED_GREEN, 0, 300, 500);
AsyncBlinker blinkLedBlue = AsyncBlinker(LED_BLUE, 0, 900, 500);
unsigned long currentTime;

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(button1Pin, INPUT);
}

void loop() {
  currentTime = millis();
  blinkLedRed.blink(1);
  if (digitalRead(button1Pin)) {
    blinkLedGreen.blink(3);
  }
  blinkLedBlue.blink(1);

  blinkLedRed.run(currentTime);
  blinkLedGreen.run(currentTime);
  blinkLedBlue.run(currentTime);
  delay(1);
}