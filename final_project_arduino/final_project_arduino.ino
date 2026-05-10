#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include <time.h>


// ========== WIFI CREDENTIALS ===========
// Note: "Maximise Compatibility" should be enabled on hotspot
// ESP32 requires 2.4GHz
const char* ssid     = "//TODO: Fill up with your hotspot username";
const char* password = "//TODO: Fill up with your hotspot password";

// ========== FIREBASE CREDENTIALS ===========
// Remember to set these before upload
#define FIREBASE_API_KEY    "//TODO: Copy and Paste the API KEY from Firebase"
#define FIREBASE_PROJECT_ID "//TODO: Copy and Paste the Project ID from Firebase"

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;

// ================== PINS ==================
#define DHTPIN       3
#define DHTTYPE      DHT11
#define BUZZER_PIN   4
#define UART_TX_PIN  17   // Transmitting: ESP32 TX -> MCXC444 RX (PTE23)
#define UART_RX_PIN  18   // Receiving: ESP32 RX <- MCXC444 TX (PTE22)

// External RGB LED pins
#define EXT_LED_RED    42
#define EXT_LED_GREEN  41
#define EXT_LED_BLUE   40

// ================== OBJECTS ===============
DHT dht(DHTPIN, DHTTYPE);
HardwareSerial mySerial(1);

// ================== RTOS ===================
QueueHandle_t dhtQueue;
SemaphoreHandle_t uartMutex;
TaskHandle_t firebaseTaskHandle = NULL;

typedef struct {
  float temperature;
  float humidity;
} TSensorData;

// ================== GLOBAL FLAGS ===========
volatile bool waterBelowThreshold = false;

volatile bool shockEventLatched = false;
volatile bool shockAlarmPending = false;
volatile bool fireEventLatched = false;
volatile bool fireAlarmPending = false;

volatile uint32_t lastDistTime = 0;
volatile bool firebaseReady = false;
volatile int waterLevelPercentage = 50;

// Latest DHT readings shared with firebaseTask
volatile float latestTemp = NAN;
volatile float latestHum  = NAN;

// ================== SETTINGS ===============
const float WATER_THRESHOLD_CM = 5.0;
const float WATER_LEVEL_BASE_CM = 30.0;

// ============= LED FUNCTIONS ===============
void allLedOff() {
  digitalWrite(EXT_LED_RED, LOW);
  digitalWrite(EXT_LED_GREEN, LOW);
  digitalWrite(EXT_LED_BLUE, LOW);
}

void setDisasterLed(bool fire, bool shock, bool water) {
  // Priority: FIRE > SHOCK > WATER
  if (fire) {
    digitalWrite(EXT_LED_RED, HIGH);
    digitalWrite(EXT_LED_GREEN, LOW);
    digitalWrite(EXT_LED_BLUE, LOW);
  }
  else if (shock) {
    digitalWrite(EXT_LED_RED, LOW);
    digitalWrite(EXT_LED_GREEN, HIGH);
    digitalWrite(EXT_LED_BLUE, LOW);
  }
  else if (water) {
    digitalWrite(EXT_LED_RED, LOW);
    digitalWrite(EXT_LED_GREEN, LOW);
    digitalWrite(EXT_LED_BLUE, HIGH);
  }
  else {
    allLedOff();
  }
}

// ====== CONNECT TO WIFI AND FIREBASE ======
void connectToFirebase() {
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting to WiFi...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        Serial.print(".");
        delay(1000);
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi failed. Status: " + String(WiFi.status()));
        return;
    }

    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

    // Anonymous auth — no user credentials required.
    // Requires Anonymous sign-in enabled in Firebase Console:
    // Authentication → Sign-in providers → Anonymous → Enable
    config.api_key = FIREBASE_API_KEY;
    config.token_status_callback = tokenStatusCallback;

    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase sign-up OK");
        firebaseReady = true;
    } else {
        Serial.println("Firebase sign-up failed: " + String(config.signer.signupError.message.c_str()));
    }

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // NTP sync — needed so the timestamp field in each history document is queryable
    // (Firestore's built-in createTime metadata cannot be used in where() queries)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    for (int i = 0; i < 20 && now < 1000000UL; i++) {
        delay(500);
        now = time(nullptr);
    }
    Serial.println(now > 1000000UL ? "NTP synced" : "NTP failed — timestamp field will be omitted");
}

// ============ WRITE TO FIREBASE ============
// Blocking call — can take 1-3s over the network
// Must only be called from firebaseTask so it never stalls other tasks
void writeToFirebase(float temperature, float humidity, int waterLevel, bool flame, bool shock) {
    if (firebaseReady && Firebase.ready()) {
        // Firestore requires explicitly typed field values (not plain JSON)
        FirebaseJson content;
        content.set("fields/temperature/doubleValue",  temperature);
        content.set("fields/humidity/doubleValue",     humidity);
        content.set("fields/waterLevel/integerValue",  waterLevel);
        content.set("fields/flame/booleanValue",       flame);
        content.set("fields/shock/booleanValue",       shock);

        // patchDocument overwrites only the listed fields, leaving any others intact.
        // "sensor/latest" is a single document used as a live snapshot for the dashboard.
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "",
                "sensor/latest", content.raw(),
                "temperature,humidity,waterLevel,flame,shock")) {
            Serial.println("Firestore updated OK");
            shockEventLatched = false;
            fireEventLatched  = false;
        } else {
            Serial.println("Firestore error: " + fbdo.errorReason());
        }

        // Append a new document to the "readings" collection for historical data.
        // timestamp field is required for dashboard queries like "last 24 hours"
        time_t now = time(nullptr);
        if (now > 1000000UL) {
            content.set("fields/timestamp/integerValue", (int)now);
        }
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "",
                "readings", content.raw())) {
            Serial.println("History record written OK");
        } else {
            Serial.println("History write error: " + fbdo.errorReason());
        }
    } else {
        Serial.println("Firebase not ready, cannot write data");
    }
}

// ================== TASK 6 =================
// Handles all Firebase uploads — periodic and event-driven.
//
// Uses ulTaskNotifyTake as a dual-purpose wait:
//   - Returns immediately if uartRecvTask sent a notification (SHOCK / FIRE)
//   - Times out after 5s and uploads anyway for the periodic heartbeat
//
// This avoids a fixed vTaskDelay, which would cause the critical event upload
// to be delayed by up to 5s depending on where in the cycle it arrived.
// Priority is kept low (1) intentionally — the network call takes 1-3s
// regardless, and a higher priority would starve uartRecvTask during that
// time, risking missed UART bytes from the MCXC444.
void firebaseTask(void *p) {
  while (1) {
    // Block until notified (critical event) or 5s elapses (periodic heartbeat).
    // pdTRUE clears the notification count on exit, preventing stacked uploads
    // if multiple events fire while a previous upload is still in progress.
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

    if (!isnan(latestTemp) && !isnan(latestHum)) {
      writeToFirebase(latestTemp, latestHum,
                waterLevelPercentage,
                fireEventLatched, shockEventLatched);
    } else {
      Serial.println("Firebase: no DHT data yet, skipping");
    }
  }
}

// ==== SIRENS FOR DIFFERENT DISASTER EVENTS =====
void playShockAlarm() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1500);
    vTaskDelay(pdMS_TO_TICKS(200));
    tone(BUZZER_PIN, 2500);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  noTone(BUZZER_PIN);
}

void playWaterAlarm() {
  for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, 800);
    vTaskDelay(pdMS_TO_TICKS(500));
    noTone(BUZZER_PIN);
    vTaskDelay(pdMS_TO_TICKS(150));

    tone(BUZZER_PIN, 800);
    vTaskDelay(pdMS_TO_TICKS(500));
    noTone(BUZZER_PIN);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void playFireAlarm() {
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, 3000);
    vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);
    vTaskDelay(pdMS_TO_TICKS(100));

    tone(BUZZER_PIN, 2000);
    vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ================== TASK 1 =================
// Poll DHT11 to get temperature and humidity reading from the sensor
void dhtTask(void *p) {
  TSensorData data;

  while (1) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      data.temperature = t;
      data.humidity = h;
      latestTemp = t;
      latestHum  = h;
      xQueueOverwrite(dhtQueue, &data);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================== TASK 2 =================
// Send temp/humidity to MCXC444
void uartSendTask(void *p) {
  TSensorData data;
  char buffer[64];

  while (1) {
    if (xQueueReceive(dhtQueue, &data, portMAX_DELAY) == pdTRUE) {
      snprintf(buffer, sizeof(buffer), "TEMP:%.1f,HUM:%.1f\n",
               data.temperature, data.humidity);

      if (xSemaphoreTake(uartMutex, portMAX_DELAY) == pdTRUE) {
        mySerial.print(buffer);
        xSemaphoreGive(uartMutex);
      }
    }
  }
}

// ================== TASK 3 =================
// Receive shock, fire and water level reading from MCXC444
void uartRecvTask(void *p) {
  static char rxBuffer[64];
  int index = 0;

  while (1) {
    while (mySerial.available()) {
      char c = mySerial.read();

      if (c == '\n' || c == '\r') {
        if (index > 0) {
          rxBuffer[index] = '\0';

          if (strcmp(rxBuffer, "SHOCK") == 0) {
            Serial.println("SHOCK detected");
              shockEventLatched = true;   // keep for Firebase
              shockAlarmPending = true;   // buzzer consumes this
            // Wake firebaseTask immediately — don't wait for the 5s heartbeat.
            // For an early-warning system, dashboard latency on critical events
            // must be minimised. Priority is NOT raised; see firebaseTask comment.
            if (firebaseTaskHandle) xTaskNotifyGive(firebaseTaskHandle);
          }
          else if (strcmp(rxBuffer, "FIRE") == 0) {
            Serial.println("FIRE detected");
            fireEventLatched = true;
            fireAlarmPending = true;
            // Same rationale as SHOCK above
            if (firebaseTaskHandle) xTaskNotifyGive(firebaseTaskHandle);
          }
          else if (strncmp(rxBuffer, "DIST:", 5) == 0) {
            float dist = atof(rxBuffer + 5);

            Serial.print("Water level distance = ");
            Serial.print(dist);
            Serial.println(" cm");

            lastDistTime = millis();
            waterBelowThreshold = (dist < WATER_THRESHOLD_CM);
            float normalisedDistance = min(dist, WATER_LEVEL_BASE_CM);
            waterLevelPercentage = ((WATER_LEVEL_BASE_CM - normalisedDistance) / WATER_LEVEL_BASE_CM) * 100;
          }

          index = 0;
        }
      } else {
        if (index < (int)sizeof(rxBuffer) - 1) {
          rxBuffer[index++] = c;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ================== TASK 4 =================
// Buzzer control for different disaster event
void buzzerTask(void *p) {
  while (1) {

    // stop water alarm if distance updates stop
    if (millis() - lastDistTime > 2000) {
      waterBelowThreshold = false;
    }

    if (fireAlarmPending) {
      fireAlarmPending = false;
      Serial.println("Playing FIRE siren");
      playFireAlarm();
    }
    else if (shockAlarmPending) {
      shockAlarmPending = false;
      Serial.println("Playing SHOCK siren");
      playShockAlarm();
    }
    else if (waterBelowThreshold) {
      Serial.println("Playing WATER siren");
      playWaterAlarm();
    }
    else {
      noTone(BUZZER_PIN);
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// ================== TASK 5 =================
// External RGB LED control for different diaster event
void ledTask(void *p) {
  while (1) {
    // If water updates stop, clear water state
    if (millis() - lastDistTime > 2000) {
      waterBelowThreshold = false;
    }

    setDisasterLed(fireAlarmPending, shockAlarmPending, waterBelowThreshold);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  connectToFirebase();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(EXT_LED_RED, OUTPUT);
  pinMode(EXT_LED_GREEN, OUTPUT);
  pinMode(EXT_LED_BLUE, OUTPUT);
  allLedOff();

  dht.begin();
  mySerial.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  dhtQueue = xQueueCreate(1, sizeof(TSensorData));
  uartMutex = xSemaphoreCreateMutex();

  xTaskCreate(dhtTask,      "dhtTask",      2048, NULL, 2, NULL);
  xTaskCreate(uartSendTask, "uartSendTask", 2048, NULL, 2, NULL);
  xTaskCreate(uartRecvTask, "uartRecvTask", 2048, NULL, 3, NULL);
  xTaskCreate(buzzerTask,   "buzzerTask",   2048, NULL, 2, NULL);
  xTaskCreate(ledTask,      "ledTask",      2048, NULL, 3, NULL);
  // Store handle so uartRecvTask can notify it directly on critical events
  xTaskCreate(firebaseTask, "firebaseTask", 8192, NULL, 1, &firebaseTaskHandle);
}

void loop() {
  // empty loop section, using FreeRTOS tasks
}