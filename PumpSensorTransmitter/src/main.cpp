#include "Arduino.h"
#include "AsyncBlinker.h"
#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

enum class Status {
  ok,
  lowAirLevel,
  noPumpActivity,
  pumpToggleOn,
  pumpToggleOff,
  noRadio,
};
Status status = Status::noPumpActivity;
Status pumpToggleState;

const int pumpSwitchPin = 6;
const int ledPinStatus = 4;
const int ledPinPumpOnOff = 2;
AsyncBlinker blinkOk = AsyncBlinker(ledPinStatus, 0, 10, 1990);
AsyncBlinker blinkLowAirLevel = AsyncBlinker(ledPinStatus, 0, 200, 100);
AsyncBlinker blinkNoPumpActivity = AsyncBlinker(ledPinStatus, 0, 2000, 100);
AsyncBlinker blinkNoRadio = AsyncBlinker(ledPinStatus, 0, 10000, 100);

int buttonState = 0;
int newPumpState = 0;
bool pumpIsOn = false;

unsigned long timeAtPumpOn;
unsigned long timeAtPumpOff;
unsigned long timeAtLastTransmit;

// Alarm for low pressure if the pump is on for less time than this duration.
// (Milliseconds).
int pumpMinOnDuration = 10000;
// Alarm for no pump activity if pump is off for longer than this.
double pumpMaxOffTime_hours = 24;
unsigned int pumpMaxOffTime = pumpMaxOffTime_hours * 3600000;

int transmitInterval = 1000 * 2;
boolean pumpIsToggled = false;
unsigned long currentTime;

// Trigger alarm when I have received more than this number of alarms in a row
int nAlarmsLimit = 3;
int nAlarms = 0;

RF24 radio(7, 8);
const byte address[6] = "00001";

void setup() {
  pinMode(pumpSwitchPin, INPUT);
  pinMode(ledPinStatus, OUTPUT);
  pinMode(ledPinPumpOnOff, OUTPUT);

  Serial.begin(9600);

  // Starting the Wireless communication
  radio.begin();
  // Setting the address where we will send the data
  radio.openWritingPipe(address);
  // You can set this as minimum or maximum depending on the
  // distance between the transmitter and receiver.
  radio.setPALevel(RF24_PA_MAX);

  // This sets the module as transmitter
  radio.stopListening();
  Serial.println("Transmitter unit");

  currentTime = millis();
  timeAtPumpOn = currentTime;
  timeAtPumpOff = currentTime;
  timeAtLastTransmit = currentTime;
}

unsigned int timeSinceLastPumpOn = 0;
unsigned int timeSinceLastPumpOff = 0;
unsigned int timeSinceLastTransmit = 0;
unsigned int pumpOnDuration = 0;
boolean readyToAlert = true;
unsigned int errorToggles = 0;
String message = "";

void loop() {
  currentTime = millis();
  newPumpState = digitalRead(pumpSwitchPin);

  if (pumpIsOn) {
    if (newPumpState == 0) {
      // The pump was toggled off now
      pumpIsOn = false;
      timeAtPumpOff = currentTime;
      Serial.println("Pump is off!");
      digitalWrite(ledPinPumpOnOff, 0);
      pumpToggleState = Status::pumpToggleOff;
      radio.write(&pumpToggleState, sizeof(pumpToggleState));

      pumpOnDuration = timeAtPumpOff - timeAtPumpOn;
      if (pumpOnDuration < pumpMinOnDuration) {
        // This is alarm
        nAlarms++;
        if (nAlarms > nAlarmsLimit) {
          message = "Low air level";
          status = Status::lowAirLevel;
        }
      } else {
        // No alarm.
        nAlarms = 0;

        message = "ok";
        status = Status::ok;
        errorToggles = 0;
      }

      // Add delay to avoid quick on/off
      delay(300);
    }
  } else { // if Pump is off
    if (newPumpState == 1) {
      // The pump was toggled on now
      pumpIsOn = true;
      timeAtPumpOn = currentTime;
      Serial.println("Pump is on!");
      digitalWrite(ledPinPumpOnOff, 1);
      pumpToggleState = Status::pumpToggleOn;
      radio.write(&pumpToggleState, sizeof(pumpToggleState));

      // Add delay to avoid quick on/off
      delay(300);
    }
  }

  timeSinceLastPumpOn = currentTime - timeAtPumpOn;
  timeSinceLastPumpOff = currentTime - timeAtPumpOff;
  timeSinceLastTransmit = currentTime - timeAtLastTransmit;

  if (timeSinceLastPumpOn > pumpMaxOffTime) {
    if (readyToAlert) {
      readyToAlert = false;
      message = "No pump activity!";
      status = Status::noPumpActivity;
    }
  } else {
    readyToAlert = true;
  }

  // Send status at a fixed interval
  if (timeSinceLastTransmit >= transmitInterval) {
    timeAtLastTransmit = currentTime;
    radio.write(&status, sizeof(status));
    Serial.println(message);
  }

  // Show status
  switch (status) {
  case Status::ok:
    blinkOk.blink(1);
    break;
  case Status::lowAirLevel:
    blinkLowAirLevel.blink(1);
    break;
  case Status::noPumpActivity:
    blinkNoPumpActivity.blink(1);
    break;
  default:
    Serial.println("Mysterious error!");
    Serial.print("state = ");
    Serial.println((int)status);

    // The LED will stay on as long as this line is executed in each loop.
    blinkOk.resetTimer();
  }

  blinkOk.run(currentTime);
  blinkLowAirLevel.run(currentTime);
  blinkNoPumpActivity.run(currentTime);

  delay(1);
}