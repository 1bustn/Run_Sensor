//All files are compiled into this one; functions remain the same
// if (logging) is added to make sure that data is logged only when the Arduino is on
// All data writes into the same SD file

#include <ArduinoLowPower.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GPS.h>

const int buttonPin = 4;  
const int ledPin = 3;      
const unsigned long holdTime = 5000; 

bool logging = false;
bool wasPressed = false;
bool holdActionDone = false; 
unsigned long pressStart = 0;

const int chipSelect = SDCARD_SS_PIN; 
const char* logFileName = "gaitlog.csv";
File logFile; 

#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);

const double minSpeedThreshold = 0.8; 
double totalDistance = 0.0; 

unsigned long lastFixTime = 0; 
bool haveLastFix = false;

const int heelPin = A1;
const int medialPin = A2;
const int lateralPin = A3;

const float stepThreshold = 0.8;   
const float resetThreshold = 0.3;   
const unsigned long stepCooldown = 150;   
const unsigned long analysisWindow = 100;  
const float balanceThreshold = 0.20; 

int stepCount = 0;
bool systemArmed = true;
bool analyzingStep = false;
unsigned long lastStepTime = 0;
unsigned long analysisStartTime = 0;

unsigned long previousMicros = 0;
const unsigned long sampleIntervalMicros = 2000; 

float maxHeel = 0;
float maxMedial = 0;
float maxLateral = 0;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(115200);

  analogReadResolution(12); 

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed to initialize");
    while (1); 
  }

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
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
    updateGPS();
    updatePiezo();
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

void updateGPS() {
  GPS.read();

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return; 
    }

    if (GPS.fix) {
      double lat = GPS.latitudeDegrees;
      double lon = GPS.longitudeDegrees;
      unsigned long now = millis();

      double speed_mps = GPS.speed * 0.514444;

      if (speed_mps < minSpeedThreshold) {
        speed_mps = 0.0;
      }

      if (haveLastFix) {
        double deltaSeconds = (now - lastFixTime) / 1000.0;
        totalDistance += speed_mps * deltaSeconds;
      }

      lastFixTime = now;
      haveLastFix = true;

      if (logFile) {
        logFile.print("GPS,");
        logFile.print(now);
        logFile.print(",");
        logFile.print(speed_mps, 3);
        logFile.print(",");
        logFile.print(totalDistance, 3);
        logFile.print(",");
        logFile.print(lat, 6);
        logFile.print(",");
        logFile.println(lon, 6);
        logFile.flush();
      }

      Serial.print("t=");
      Serial.print(now);
      Serial.print("  speed=");
      Serial.print(speed_mps, 3);
      Serial.print(" m/s  dist=");
      Serial.print(totalDistance, 3);
      Serial.print(" m  lat=");
      Serial.print(lat, 6);
      Serial.print("  lon=");
      Serial.println(lon, 6);
    } else {
      Serial.println("Waiting for GPS fix...");
    }
  }
}

void updatePiezo() {
  unsigned long currentMicros = micros();
  unsigned long currentTime = millis();

  if (currentMicros - previousMicros < sampleIntervalMicros) {
    return; 
  }
  previousMicros = currentMicros;

  int heelRaw = analogRead(heelPin);
  int medialRaw = analogRead(medialPin);
  int lateralRaw = analogRead(lateralPin);

  float heelVoltage = heelRaw * (3.3 / 4095.0);
  float medialVoltage = medialRaw * (3.3 / 4095.0);
  float lateralVoltage = lateralRaw * (3.3 / 4095.0);

  if (!systemArmed && !analyzingStep) {
    if (heelVoltage < resetThreshold) {
      systemArmed = true;
    }
  }

  if (systemArmed && !analyzingStep && heelVoltage >= stepThreshold && currentTime - lastStepTime >= stepCooldown) {
    stepCount++;
    Serial.println();
    Serial.println("========== STEP ==========");
    Serial.print("Step Count: ");
    Serial.println(stepCount);

    systemArmed = false;
    analyzingStep = true;
    analysisStartTime = currentTime;

    maxHeel = heelVoltage;
    maxMedial = medialVoltage;
    maxLateral = lateralVoltage;
    lastStepTime = currentTime;
  }

  if (analyzingStep) {
    if (heelVoltage > maxHeel) maxHeel = heelVoltage;
    if (medialVoltage > maxMedial) maxMedial = medialVoltage;
    if (lateralVoltage > maxLateral) maxLateral = lateralVoltage;

    if (currentTime - analysisStartTime >= analysisWindow) {
      analyzingStep = false;

      float totalForefoot = maxMedial + maxLateral;
      float balanceScore = 0;
      if (totalForefoot > 0.01) {
        balanceScore = (maxMedial - maxLateral) / totalForefoot;
      }

      Serial.print("Heel Peak: ");
      Serial.print(maxHeel, 3);
      Serial.println(" V");
      Serial.print("Medial Peak: ");
      Serial.print(maxMedial, 3);
      Serial.println(" V");
      Serial.print("Lateral Peak: ");
      Serial.print(maxLateral, 3);
      Serial.println(" V");
      Serial.print("Balance Score: ");
      Serial.println(balanceScore, 3);

      String pattern;
      String interpretation;
      if (balanceScore > balanceThreshold) {
        pattern = "MEDIAL";
        interpretation = "PRONATION TENDENCY";
      } else if (balanceScore < -balanceThreshold) {
        pattern = "LATERAL";
        interpretation = "SUPINATION TENDENCY";
      } else {
        pattern = "BALANCED";
        interpretation = "NEUTRAL / BALANCED";
      }

      Serial.print("Loading Pattern: ");
      Serial.println(pattern);
      Serial.print("Interpretation: ");
      Serial.println(interpretation);
      Serial.println("==========================");

      if (logFile) {
        logFile.print("STEP,");
        logFile.print(currentTime);
        logFile.print(",");
        logFile.print(stepCount);
        logFile.print(",");
        logFile.print(maxHeel, 3);
        logFile.print(",");
        logFile.print(maxMedial, 3);
        logFile.print(",");
        logFile.print(maxLateral, 3);
        logFile.print(",");
        logFile.print(balanceScore, 3);
        logFile.print(",");
        logFile.println(interpretation);
        logFile.flush();
      }
    }
  }
}
