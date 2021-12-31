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
Status status = Status::noRadio;
Status lastStatus = status;
Status x = status;

// Warn if no radio is received for this duration (milliseconds)
unsigned long maxRadioReceiveInterval = 1000 * 5;

const int ledPinStatus = 2;
const int ledPinLowAirLevel = 3;
const int ledPinPumpOnOff = 4;
const int ledPinNoPumpActivity = 5;
const int ledPinNoRadio = 6;

AsyncBlinker blinkOk = AsyncBlinker(ledPinStatus, 0, 10, 1990);
AsyncBlinker blinkLowAirLevel = AsyncBlinker(ledPinStatus, 0, 200, 100);
AsyncBlinker blinkNoPumpActivity = AsyncBlinker(ledPinStatus, 0, 2000, 100);
AsyncBlinker blinkNoRadio = AsyncBlinker(ledPinStatus, 0, 10000, 100);

int buttonState = 0;
// int xState = 0;
unsigned long currentTime;
unsigned long lastRadioReceiveTime = currentTime;
unsigned long timeSinceLastRadioReceive;

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

void setup() {
  pinMode(ledPinStatus, OUTPUT);
  pinMode(ledPinLowAirLevel, OUTPUT);
  pinMode(ledPinPumpOnOff, OUTPUT);
  pinMode(ledPinNoPumpActivity, OUTPUT);
  pinMode(ledPinNoRadio, OUTPUT);

  Serial.begin(9600);

  // Starting the Wireless communication
  radio.begin();
  // Setting the address where we will send and the data
  radio.openWritingPipe(address);
  // You can set this as minimum or maximum depending on the
  // distance between the transmitter and receiver.
  radio.setPALevel(RF24_PA_MAX);

  // This sets the module as receiver
  radio.openReadingPipe(0, address);
  radio.startListening();
  Serial.println("Receiver unit");
}


void loop() {
  currentTime = millis();

  // Get pump state
  if (radio.available()) {
    lastRadioReceiveTime = millis();
    radio.read(&x, sizeof(x));

    switch (x) {
    case Status::pumpToggleOn:
      Serial.println("Pump is on!");
      digitalWrite(ledPinPumpOnOff, 1);
      break;
    case Status::pumpToggleOff:
      Serial.println("Pump is off!");
      digitalWrite(ledPinPumpOnOff, 0);
      break;
    default:
      status = x;
      break;
    }
  }

  timeSinceLastRadioReceive = currentTime - lastRadioReceiveTime;
  if (timeSinceLastRadioReceive > maxRadioReceiveInterval) {
    status = Status::noRadio;
  }

  // Show state
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
  case Status::noRadio:
    blinkNoRadio.blink(1);
    break;
  default:
    Serial.println("Mysterious error!");
    Serial.print("state = ");
    Serial.println((int)status);

    // The LED will stay on as long as this line is executed in each loop.
    blinkOk.resetTimer();
  }

  digitalWrite(ledPinLowAirLevel, status == Status::lowAirLevel);
  digitalWrite(ledPinNoPumpActivity, status == Status::noPumpActivity);
  digitalWrite(ledPinNoRadio, status == Status::noRadio);

  blinkOk.run(currentTime);
  blinkLowAirLevel.run(currentTime);
  blinkNoPumpActivity.run(currentTime);
  blinkNoRadio.run(currentTime);

  delay(1);


}