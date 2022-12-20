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
bool pumpState = false;
bool newPumpState = false;
bool pumpIsOn = false;

unsigned long timeAtPumpOn;
unsigned long timeAtPumpOff;
unsigned long timeAtLastTransmit;

// Alarm for low pressure if the pump is on for less time than this duration.
// (Milliseconds).
const int pumpMinOnDuration = 10000;
// Alarm for no pump activity if pump is off for longer than this.
const double pumpMaxOffTime_hours = 12;
const unsigned long pumpMaxOffTime = pumpMaxOffTime_hours * 3600000;

const int transmitInterval = 1000 * 2;
boolean pumpIsToggled = false;
unsigned long currentTime;

// Trigger alarm when I have received more than this number of alarms in a row
const int nAlarmsLimit = 2;
unsigned long nAlarms = 0;
unsigned long nOK = 0;

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

unsigned long timeSinceLastPumpOn = 0;
unsigned long timeSinceLastPumpOff = 0;
unsigned long timeSinceLastTransmit = 0;
unsigned long totalPumpOnDuration = 0;
boolean has_changed_to_no_pump_activity = false;
String message = "Status: No pump activity!";

void loop() {
  currentTime = millis();
  newPumpState = digitalRead(pumpSwitchPin);

  if (pumpState != newPumpState) {
    // The pump was toggled
    pumpState = newPumpState;

    // Add delay to avoid quick on/off
    delay(500);

    if (pumpState == true) {
      // The pump was toggled on
      Serial.println("Pump is on!");
      pumpToggleState = Status::pumpToggleOn;
      timeAtPumpOn = currentTime;
    } else {
      // The pump was toggled off
      Serial.println("Pump is off!");
      pumpToggleState = Status::pumpToggleOff;
      timeAtPumpOff = currentTime;

      totalPumpOnDuration = timeAtPumpOff - timeAtPumpOn;

      if (totalPumpOnDuration < pumpMinOnDuration) {
        // This is low air level
        nAlarms++;
        if (nAlarms >= nAlarmsLimit) {
          message = "Status: Low air level";
          status = Status::lowAirLevel;
          nOK = 0;
        }
      } else {
        // This is OK
        nOK++;
        if (nOK > nAlarms) {
          message = "Status: OK";
          status = Status::ok;
          nAlarms = 0;
        }
      }
    }
    digitalWrite(ledPinPumpOnOff, pumpState);
    radio.write(&pumpToggleState, sizeof(pumpToggleState));
  }

  timeSinceLastPumpOn = currentTime - timeAtPumpOn;
  timeSinceLastPumpOff = currentTime - timeAtPumpOff;
  timeSinceLastTransmit = currentTime - timeAtLastTransmit;

  if (timeSinceLastPumpOn > pumpMaxOffTime) {
    if (!has_changed_to_no_pump_activity) {
      message = "Status: No pump activity!";
      status = Status::noPumpActivity;
      has_changed_to_no_pump_activity = true;
    }
  } 
  else {
    has_changed_to_no_pump_activity = false;
  }

  // Send status at a fixed interval
  if (timeSinceLastTransmit >= transmitInterval) {
    timeAtLastTransmit = currentTime;
    radio.write(&status, sizeof(status));
    radio.write(&pumpToggleState, sizeof(pumpToggleState));
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