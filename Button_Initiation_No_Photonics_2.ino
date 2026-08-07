// Button controlled data logging
// To start logging, hold button for 5+ seconds, LED will turn on as an indicator
// To stop logging, hold button for 5+ seconds again and LED will turn off; the board goes into sleep mode, all recording stops, file receives no appends
// To wake the board and begin logging, hold for 5+ seconds and LED will turn on again, wakes from sleep mode and resumes normal function

#include <ArduinoLowPower.h>
#include <SD.h>
#include <SPI.h>

const int buttonPin = 4;   
const int ledPin = 3;      
const unsigned long holdTime = 5000; 

const int chipSelect = SDCARD_SS_PIN; 
const char* logFileName = "gaitlog.csv";
File logFile; 

bool logging = false;
bool wasPressed = false;
bool holdActionDone = false; 
unsigned long pressStart = 0;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(115200);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed to initialize");
    while (1); 
  }
}

void loop() {
  bool isPressed = (digitalRead(buttonPin) == LOW);

  if (isPressed) {
    if (!wasPressed) {
      pressStart = millis();
      wasPressed = true;
      holdActionDone = false;
    } else if (!holdActionDone && (millis() - pressStart >= holdTime)) {
      holdActionDone = true; 
      handleLongPress();
    }
  } else {
    wasPressed = false;
  }

  if (logging) {
    // Code to record data goes in here once done
  }
}

void handleLongPress() {
  if (!logging) {
    logging = true;

    logFile = SD.open(logFileName, FILE_WRITE);
    if (!logFile) {
      Serial.println("Failed to open log file");
    } else {
      logFile.println("session started");
    }

    digitalWrite(ledPin, HIGH);
    Serial.println("Logging started");
  } else {
    logging = false;

    if (logFile) {
      logFile.println("session stopped");
      logFile.close(); 
    }

    digitalWrite(ledPin, LOW);
    Serial.println("Logging stopped, sleeping now");

    goToSleep();
  }
}

void goToSleep() {
  LowPower.attachInterruptWakeup(buttonPin, wakeUp, FALLING);
  LowPower.deepSleep();
  pressStart = millis();
  wasPressed = true;
  holdActionDone = false;
}

void wakeUp() {
}
