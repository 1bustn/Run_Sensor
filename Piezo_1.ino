// Peak voltages are collected and compared to obtain pronation/supination tendency through the measurement "balance", not exact degree though due to hardware limitations
// Step count is from the heel pin detecting voltage spike from the piezo disk going above 0.8 volts, then requires the voltage to drop below 0.3 volts to be able to register the next step
// The reset voltage can be adjusted depending on the baseline voltage from the piezo in order to be able to accurately get new steps
// Samples every 2 ms so that piezo spikes can be captured with a small enough time window whilst the other code in the loop can run properly in time
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
  Serial.begin(115200);
  
  analogReadResolution(12);
  
  Serial.println();
  Serial.println("================================");
  Serial.println(" 3-Piezo Step/Gait Tracker");
  Serial.println("================================");
  Serial.println("A1 = Heel");
  Serial.println("A2 = Medial (Left)");
  Serial.println("A3 = Lateral (Right)");
  Serial.println();
  Serial.println("System ready.");
  Serial.println();
}

void loop() {

  unsigned long currentMicros = micros();
  unsigned long currentTime = millis();

  if (currentMicros - previousMicros >= sampleIntervalMicros) {
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
      if (heelVoltage > maxHeel) {
        maxHeel = heelVoltage;
      }

      if (medialVoltage > maxMedial) {
        maxMedial = medialVoltage;
      }
      
      if (lateralVoltage > maxLateral) {
        maxLateral = lateralVoltage;
      }

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

        if (balanceScore > balanceThreshold) {
          Serial.println("Loading Pattern: MEDIAL");
          Serial.println("Interpretation: PRONATION TENDENCY");
        } else if (balanceScore < -balanceThreshold) {
          Serial.println("Loading Pattern: LATERAL");
          Serial.println("Interpretation: SUPINATION TENDENCY");
        } else {
          Serial.println("Loading Pattern: BALANCED");
          Serial.println("Interpretation: NEUTRAL / BALANCED");
        }
        Serial.println("==========================");
      }
    }
  } 
}