// Uses Adafruit Ultimate GPS Breakout v3 to get data
// Speed comes straight from the GPS module's own Doppler based speed output
// Distance is from speed/time, not coordinates
// Coordinates are here for reference

#include <Adafruit_GPS.h>
#include <SD.h>
#include <SPI.h>

#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);

const int chipSelect = SDCARD_SS_PIN; 
const char* logFileName = "gaitgps.csv";

const double minSpeedThreshold = 0.8;

double totalDistance = 0.0;   
unsigned long lastFixTime = 0; 
bool haveLastFix = false;

void setup() {
  Serial.begin(115200);

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed to initialize");
    while (1); 
  }

  File dataFile = SD.open(logFileName, FILE_WRITE);
  if (dataFile) {
    dataFile.println("timestamp_ms,speed_mps,distance_m,latitude,longitude");
    dataFile.close();
  }
}

void loop() {
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

      logToSD(now, speed_mps, totalDistance, lat, lon);

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

void logToSD(unsigned long timestamp, double speed, double distance, double lat, double lon) {
  File dataFile = SD.open(logFileName, FILE_WRITE);
  if (dataFile) {
    dataFile.print(timestamp);
    dataFile.print(",");
    dataFile.print(speed, 3);
    dataFile.print(",");
    dataFile.print(distance, 3);                                                                       
    dataFile.print(",");
    dataFile.print(lat, 6);
    dataFile.print(",");
    dataFile.println(lon, 6);
    dataFile.close();
  }
}
