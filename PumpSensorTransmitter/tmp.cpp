

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
}

void AsyncBlinker::resetTimer() {
  m_timeSinceLastCycle = 0;
  m_timeAtLastRun = millis();
}

////////////// AsyncBlinker end ///////////////////


enum class Status {
  ok,
  lowAirLevel,
  noPumpActivity,
  pumpToggleOn,
  pumpToggleOff,
};
Status status = Status::noPumpActivity;
Status pumpToggleState;

const int pumpSwitchPin = 6;
const int ledPinStatus = 4;
const int ledPinPumpOnOff = 2;
AsyncBlinker blinkOk = AsyncBlinker(ledPinStatus, 0, 10, 1990);
AsyncBlinker blinkLowAirLevel = AsyncBlinker(ledPinStatus, 0, 200, 100);
AsyncBlinker blinkNoPumpActivity = AsyncBlinker(ledPinStatus, 0, 2000, 100);

int buttonState = 0;
bool pumpState = false;
bool newPumpState = false;
bool pumpIsOn = false;

unsigned long timeAtPumpOn;
unsigned long timeAtPumpOff;
unsigned long timeAtLastTransmit;

// Alarm for low pressure if the pump is on for less time than this duration.
// (Milliseconds).
int pumpMinOnDuration = 10000;
// Alarm for no pump activity if pump is off for longer than this.
double pumpMaxOffTime_hours = 24;
unsigned long pumpMaxOffTime = pumpMaxOffTime_hours * 3600000;

int transmitInterval = 1000 * 2;
boolean pumpIsToggled = false;
unsigned long currentTime;

// Trigger alarm when I have received more than this number of alarms in a row
int nAlarmsLimit = 2;
unsigned long nAlarms = 0;
unsigned long nOK = 0;

const byte address[6] = "00001";

void setup() {
  pinMode(pumpSwitchPin, INPUT);
  pinMode(ledPinStatus, OUTPUT);
  pinMode(ledPinPumpOnOff, OUTPUT);

  Serial.begin(9600);

  // Starting the Wireless communication
  // Setting the address where we will send the data
  // You can set this as minimum or maximum depending on the
  // distance between the transmitter and receiver.

  // This sets the module as transmitter
  Serial.println("Transmitter unit");

  currentTime = millis();
  timeAtPumpOn = currentTime;
  timeAtPumpOff = currentTime;
  timeAtLastTransmit = currentTime;
}

unsigned int timeSinceLastPumpOn = 0;
unsigned int timeSinceLastPumpOff = 0;
unsigned int timeSinceLastTransmit = 0;
unsigned int totalPumpOnDuration = 0;
boolean readyToAlert = true;
String message = "Status: No pump activity!";

void loop() {
  currentTime = millis();
  newPumpState = digitalRead(pumpSwitchPin);

  if (pumpState != newPumpState) {
    // The pump was toggled
    pumpState = newPumpState;

    // Add delay to avoid quick on/off
    delay(300);

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
        if (nAlarms > nAlarmsLimit) {
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
  }

  timeSinceLastPumpOn = currentTime - timeAtPumpOn;
  timeSinceLastPumpOff = currentTime - timeAtPumpOff;
  timeSinceLastTransmit = currentTime - timeAtLastTransmit;

  if (timeSinceLastPumpOn > pumpMaxOffTime) {
    if (readyToAlert) {
      readyToAlert = false;
      message = "Status: No pump activity!";
      status = Status::noPumpActivity;
    }
  } else {
    readyToAlert = true;
  }

  // Send status at a fixed interval
  if (timeSinceLastTransmit >= transmitInterval) {
    timeAtLastTransmit = currentTime;
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