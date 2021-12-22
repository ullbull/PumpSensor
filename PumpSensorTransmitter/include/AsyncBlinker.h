//
// Created by UlrikKarlsson on 2021-12-08.
//

#ifndef TEST_ASYNCBLINKER_H
#define TEST_ASYNCBLINKER_H

#include "Arduino.h"

class AsyncBlinker {
private:
    int m_pin;
    int m_onTime;
    int m_offTime;
    unsigned long m_timeAtLastRun;
    unsigned long m_timeSinceLastCycle;
    int m_numberOfBlinks = 0;
public:
    AsyncBlinker(int pin, int offTime, int onTime);

    ~AsyncBlinker();

    void setOnTime(int onTime);

    void setOffTime(int offTime);

    // This method must be called on every round in main loop
    void run(unsigned long currentTime);

    // Call this method when you want to blink the LED
    void blink(int numberOfBlinks);

    void resetTimer();
};


#endif //TEST_ASYNCBLINKER_H
