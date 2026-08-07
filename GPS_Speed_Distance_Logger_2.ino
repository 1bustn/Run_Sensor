// GPS based speed and distance logging to SD card (MKR Zero)
// Uses Adafruit Ultimate GPS Breakout v3
//
// While the Arduino is on, it continuously reads GPS coordinates,
// calculates speed and distance traveled from those coordinates,
// and logs timestamp, speed, distance, and coordinates to the SD card.
// Called #2 because the first version didn't print into the serial monitor.

#include <Adafruit_GPS.h>
#include <SD.h>
#include <SPI.h>

#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);

const int chipSelect = SDCARD_SS_PIN; // MKR Zero's onboard SD slot
const char* logFileName = "gaitgps.csv";

double totalDistance = 0.0;   // running total distance, meters
double lastLat = 0.0;
double lastLon = 0.0;
unsigned long lastFixTime = 0; // millis() at the previous fix, used as the stopwatch reference
bool haveLastFix = false;

void setup() {
  Serial.begin(115200);

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed to initialize");
    while (1); // stop here if SD isn't working, nothing to log to
  }

  // write the column headers once at startup
  File dataFile = SD.open(logFileName, FILE_WRITE);
  if (dataFile) {
    dataFile.println("timestamp_ms,speed_mps,distance_m,latitude,longitude");
    dataFile.close();
  }
}

void loop() {
  // keep feeding characters into the GPS parser
  GPS.read();

  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return; // sentence didn't parse cleanly, skip it
    }

    if (GPS.fix) {
      double lat = GPS.latitudeDegrees;
      double lon = GPS.longitudeDegrees;
      unsigned long now = millis(); // stopwatch, time since Arduino turned on

      double speed_mps = 0.0;
      double distanceStep = 0.0;

      if (haveLastFix) {
        distanceStep = haversineDistance(lastLat, lastLon, lat, lon);
        totalDistance += distanceStep;

        double deltaSeconds = (now - lastFixTime) / 1000.0;
        if (deltaSeconds > 0) {
          speed_mps = distanceStep / deltaSeconds;
        }
      }

      lastLat = lat;
      lastLon = lon;
      lastFixTime = now;
      haveLastFix = true;

      logToSD(now, speed_mps, totalDistance, lat, lon);

      // print the same data to Serial so you can watch it live
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
      // no fix yet, print a heartbeat so you know it's still running
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

// straight line distance between two GPS coordinates, in meters
double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0; // Earth's radius in meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}
