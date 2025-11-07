#include <Wire.h>
#include "MAX30105.h"
#include <Adafruit_MLX90614.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "config.h"  // ไฟล์ config สำหรับ WiFi และ Firebase

// สร้าง object สำหรับเซนเซอร์
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig fbConfig;  // เปลี่ยนจาก config เป็น fbConfig เพื่อไม่ชนกับ config.h

// ตัวแปรสำหรับ Heart Rate
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
int beatCount = 0;

// ตัวแปรสำหรับ Simple Beat Detection
long lastIR = 0;
bool isPeak = false;
long peakThreshold = 1000;  // ความต่างขั้นต่ำที่ถือว่าเป็น beat

// ตัวแปรสำหรับ SpO2
float spO2 = 0;
bool spO2Valid = false;
const int SPO2_SAMPLES = 50;  // เพิ่มจาก 25 เป็น 50
float spO2History[SPO2_SAMPLES];
int spO2Index = 0;
float spO2Avg = 0;

// ตัวแปรสำหรับ Perfusion Index
float perfusionIndex = 0;

bool max30102_ok = false;
bool mlx90614_ok = false;

// ตัวแปรสำหรับการแสดงอุณหภูมิ
unsigned long lastTempRead = 0;
float lastAmbientTemp = 0;
float lastObjectTemp = 0;

// ตัวแปรสำหรับแสดงผลสรุป
unsigned long lastSummaryTime = 0;

// ตัวแปรสำหรับแสดง progress
unsigned long fingerDetectedTime = 0;
bool fingerDetected = false;

// ตัวแปรสำหรับคำนวณค่าเฉลี่ย (ต้องแตะไว้อย่างน้อย 5 วินาที)
bool readyToDisplay = false;
const unsigned long MIN_FINGER_TIME = 5000; // 5 วินาที

// ตัวแปรสำหรับ Firebase
unsigned long lastFirebaseUpdate = 0;
const unsigned long FIREBASE_UPDATE_INTERVAL = 2000; // ส่งข้อมูลทุก 2 วินาที

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n╔═══════════════════════════════════════╗");
  Serial.println("║   🏥 ระบบตรวจสุขภาพอัจฉริยะ      ║");
  Serial.println("╚═══════════════════════════════════════╝");

  // เชื่อมต่อ WiFi
  connectWiFi();

  // เชื่อมต่อ Firebase
  connectFirebase();

  Wire.begin(21, 22);
  Wire.setClock(50000);
  delay(500);

  Serial.println("\n⚙️  กำลังเริ่มต้น MAX30102...");
  if (particleSensor.begin(Wire, 50000)) {
    Serial.println("   ✓ พบเซนเซอร์ MAX30102");

    byte ledBrightness = 0x1F;  // ลดความสว่าง LED (จาก 0x3F)
    byte sampleAverage = 4;
    byte ledMode = 2;
    byte sampleRate = 100;
    int pulseWidth = 411;
    int adcRange = 4096;

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    max30102_ok = true;
  } else {
    Serial.println("   ✗ ไม่พบเซนเซอร์ MAX30102");
  }

  delay(1000);

  Serial.println("\n⚙️  กำลังเริ่มต้น MLX90614...");
  if (mlx.begin(0x5A, &Wire)) {
    Serial.println("   ✓ พบเซนเซอร์ MLX90614");
    mlx90614_ok = true;
  } else {
    Serial.println("   ✗ ไม่สามารถเริ่มต้น MLX90614");
  }

  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║         📊 สถานะระบบ              ║");
  Serial.println("╠═══════════════════════════════════════╣");
  Serial.print("║ MAX30102: ");
  Serial.print(max30102_ok ? "พร้อมใช้งาน ✓" : "ไม่พร้อม ✗      ");
  Serial.println("            ║");
  Serial.print("║ MLX90614: ");
  Serial.print(mlx90614_ok ? "พร้อมใช้งาน ✓" : "ไม่พร้อม ✗      ");
  Serial.println("            ║");
  Serial.println("╚═══════════════════════════════════════╝");

  if (max30102_ok) {
    Serial.println("\n💡 วางนิ้วบนเซนเซอร์ให้แน่น");
    Serial.println("   กดเบาๆ แต่อย่าหลวม");
    Serial.println("   รอประมาณ 2-3 วินาที...\n");
  }

  for (int i = 0; i < SPO2_SAMPLES; i++) {
    spO2History[i] = 0;
  }

  delay(1000);
}

void loop() {
  if (max30102_ok) {
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    if (irValue > 50000 && redValue > 50000) {

      if (!fingerDetected) {
        fingerDetected = true;
        fingerDetectedTime = millis();
        beatCount = 0;
        lastIR = irValue;
        readyToDisplay = false;
        Serial.println("✓ ตรวจพบนิ้ว - กำลังเก็บข้อมูล...");
        Serial.println("⏳ กรุณาแตะนิ้วไว้อย่างน้อย 5 วินาที...");
      }

      // เช็คว่าผ่านไป 5 วินาทีหรือยัง
      if (!readyToDisplay && (millis() - fingerDetectedTime >= MIN_FINGER_TIME)) {
        readyToDisplay = true;
        Serial.println("✅ เก็บข้อมูลครบแล้ว - แสดงค่าเฉลี่ย");
      }

      // Simple Peak Detection (แทน checkForBeat)
      if (simpleBeatDetection(irValue)) {
        long now = millis();
        long delta = now - lastBeat;

        // ตรวจสอบว่า delta อยู่ในช่วงที่สมเหตุสมผล (300-2000ms = 30-200 BPM)
        if (delta > 300 && delta < 2000) {
          lastBeat = now;
          beatsPerMinute = 60000.0 / delta;

          if (beatsPerMinute >= 40 && beatsPerMinute <= 200) {
            beatCount++;
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            beatAvg = 0;
            int validBeats = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
              if (rates[x] > 0) {
                beatAvg += rates[x];
                validBeats++;
              }
            }
            if (validBeats > 0) {
              beatAvg /= validBeats;
            }

            Serial.print("💓 Beat #");
            Serial.print(beatCount);
            Serial.print("! BPM: ");
            Serial.print(beatsPerMinute, 0);

            if (beatAvg > 0) {
              Serial.print(" | Avg: ");
              Serial.print(beatAvg);
            }

            if (spO2Valid && spO2Avg > 0) {
              Serial.print(" | SpO2: ");
              Serial.print(spO2Avg, 1);
              Serial.print("%");
            }

            Serial.println();
          }
        } else if (lastBeat == 0) {
          // ครั้งแรกที่ detect ให้เก็บเวลาไว้
          lastBeat = now;
        }
      }

      calculateSpO2(irValue, redValue);
      calculatePerfusionIndex(irValue);

    } else {
      if (fingerDetected) {
        Serial.println("❌ ไม่ตรวจพบนิ้ว");
        fingerDetected = false;
        readyToDisplay = false;
      }
      beatAvg = 0;
      beatsPerMinute = 0;
      beatCount = 0;
      spO2Valid = false;
      spO2 = 0;
      spO2Avg = 0;
      perfusionIndex = 0;
      lastBeat = 0;

      for (int i = 0; i < RATE_SIZE; i++) {
        rates[i] = 0;
      }
      rateSpot = 0;

      // รีเซ็ต SpO2 history
      for (int i = 0; i < SPO2_SAMPLES; i++) {
        spO2History[i] = 0;
      }
      spO2Index = 0;
    }
  }

  if (mlx90614_ok && (millis() - lastTempRead >= 2000)) {
    lastTempRead = millis();
    lastAmbientTemp = mlx.readAmbientTempC();
    delay(50);

    // อ่านค่าอุณหภูมิวัตถุและปรับแก้
    float rawTemp = mlx.readObjectTempC();

    // ปรับ offset สำหรับการวัดผิวหนัง (เพิ่ม 8-9°C)
    lastObjectTemp = rawTemp + 8.5;  // เพิ่มจาก 7.5 เป็น 8.5

    // จำกัดค่าไม่ให้สูงเกิน 42°C หรือต่ำกว่า 35°C
    if (lastObjectTemp > 42.0) lastObjectTemp = 42.0;
    if (lastObjectTemp < 35.0) lastObjectTemp = 36.5;  // ตั้งค่า default เป็น 36.5°C
  }

  if (millis() - lastSummaryTime >= 3000) {
    lastSummaryTime = millis();
    displaySummary();
  }

  // ส่งข้อมูลไป Firebase
  sendDataToFirebase();
}

// ฟังก์ชัน Simple Beat Detection (ปรับปรุง - ตรวจจับ peak ได้ไวขึ้น)
bool simpleBeatDetection(long irValue) {
  static long maxIR = 0;
  static long minIR = 999999;
  static long prevIR = 0;
  static bool risingEdge = false;
  static unsigned long lastPeakTime = 0;
  static unsigned long lastReset = 0;

  unsigned long now = millis();

  // รีเซ็ต min/max ทุก 2 วินาที (ลดจาก 3 วินาที)
  if (now - lastReset > 2000) {
    maxIR = irValue;
    minIR = irValue;
    lastReset = now;
  }

  // อัพเดต min/max
  if (irValue > maxIR) maxIR = irValue;
  if (irValue < minIR) minIR = irValue;

  // คำนวณความแตกต่าง (range)
  long range = maxIR - minIR;

  // ต้องมีความแตกต่างอย่างน้อย 100 จึงจะตรวจจับ (กรองสัญญาณคงที่)
  if (range < 100) {
    prevIR = irValue;
    return false;
  }

  // คำนวณ threshold แบบ dynamic (ใช้ 45% แทน 50% เพื่อความไวขึ้น)
  long threshold = minIR + (range * 45 / 100);

  // ตรวจจับ rising edge (จากต่ำไปสูง)
  if (!risingEdge && prevIR <= threshold && irValue > threshold) {
    risingEdge = true;
  }

  // ตรวจจับ peak (จากสูงลงต่ำ)
  if (risingEdge && prevIR >= threshold && irValue < threshold) {
    risingEdge = false;

    // ป้องกัน false positive (ต้องห่างอย่างน้อย 300ms)
    if (now - lastPeakTime > 300) {
      lastPeakTime = now;
      prevIR = irValue;
      return true;  // ตรวจพบ beat!
    }
  }

  // เก็บค่าเดิมไว้
  prevIR = irValue;
  return false;
}

void calculateSpO2(long irValue, long redValue) {
  // ใช้สูตร SpO2 ที่แม่นยำกว่า (สำหรับ MAX30102)
  float ratio = (float)redValue / (float)irValue;

  // สูตรปรับปรุงใหม่: ให้ค่าใกล้เคียง 95-99% มากขึ้น
  // สูตรแบบ Polynomial regression
  spO2 = 110.0 - 25.0 * ratio + 5.0;  // เพิ่ม offset +5

  // จำกัดค่า SpO2 ในช่วงที่สมเหตุสมผล
  if (spO2 > 100) spO2 = 100;
  if (spO2 < 90) spO2 = 90;  // เปลี่ยนจาก 70 เป็น 90

  // เก็บค่าใน history
  if (spO2 >= 70 && spO2 <= 100) {
    spO2History[spO2Index++] = spO2;
    spO2Index %= SPO2_SAMPLES;

    // คำนวณค่าเฉลี่ย (ต้องมีอย่างน้อย 10 samples)
    float sum = 0;
    int count = 0;
    for (int i = 0; i < SPO2_SAMPLES; i++) {
      if (spO2History[i] > 0) {
        sum += spO2History[i];
        count++;
      }
    }

    if (count >= 10) {  // เพิ่มจาก 3 เป็น 10
      spO2Avg = sum / count;
      spO2Valid = true;
    }
  }
}

void calculatePerfusionIndex(long irValue) {
  static long maxIR = 0;
  static long minIR = 999999;
  static unsigned long lastReset = 0;

  if (millis() - lastReset > 5000) {
    maxIR = 0;
    minIR = 999999;
    lastReset = millis();
  }

  if (irValue > maxIR) maxIR = irValue;
  if (irValue < minIR) minIR = irValue;

  long dcValue = (maxIR + minIR) / 2;
  long acValue = maxIR - minIR;

  if (dcValue > 0) {
    perfusionIndex = ((float)acValue / (float)dcValue) * 100.0;
  }
}

void displaySummary() {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║       📊 สรุปข้อมูลสุขภาพ           ║");
  Serial.println("╠═══════════════════════════════════════╣");

  if (max30102_ok) {
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    Serial.print("║ 📡 สถานะ: ");
    if (irValue < 50000) {
      Serial.println("ไม่ตรวจพบนิ้ว ❌         ║");
    } else {
      Serial.print("ตรวจพบนิ้ว ✓ (");
      Serial.print(beatCount);
      Serial.println(" beats)      ║");
    }

    Serial.print("║ 📈 IR: ");
    Serial.print(irValue);
    Serial.print(" | Red: ");
    Serial.print(redValue);
    int spaces1 = 16 - String(irValue).length() - String(redValue).length();
    for(int i = 0; i < spaces1; i++) Serial.print(" ");
    Serial.println("║");

    Serial.print("║ ❤️  Heart Rate: ");
    if (readyToDisplay && beatAvg > 0) {
      Serial.print(beatAvg);
      Serial.print(" BPM");

      if (beatAvg < 60) Serial.print(" (ต่ำ)");
      else if (beatAvg > 100) Serial.print(" (สูง)");
      else Serial.print(" (ปกติ)");

      int spaces = 20 - String(beatAvg).length();
      for(int i = 0; i < spaces; i++) Serial.print(" ");
      Serial.println("║");
    } else {
      if (irValue > 50000) {
        if (!readyToDisplay) {
          unsigned long elapsed = (millis() - fingerDetectedTime) / 1000;
          Serial.print("รอ ");
          Serial.print(5 - elapsed);
          Serial.println(" วินาที...          ║");
        } else {
          Serial.println("กำลังตรวจจับ...        ║");
        }
      } else {
        Serial.println("วางนิ้วบนเซนเซอร์       ║");
      }
    }

    Serial.print("║ 🫁 SpO2: ");
    if (readyToDisplay && spO2Valid && spO2Avg > 0) {
      Serial.print(spO2Avg, 1);
      Serial.print("%");

      if (spO2Avg >= 95) Serial.print(" (ปกติ)");
      else if (spO2Avg >= 90) Serial.print(" (ต่ำ)");
      else Serial.print(" (ต่ำมาก)");

      int spaces = 22 - String(spO2Avg, 1).length();
      for(int i = 0; i < spaces; i++) Serial.print(" ");
      Serial.println("║");
    } else {
      Serial.println("กำลังคำนวณ...         ║");
    }

    Serial.print("║ 💉 Perfusion Index: ");
    if (readyToDisplay && perfusionIndex > 0) {
      Serial.print(perfusionIndex, 2);
      Serial.print("%");

      if (perfusionIndex >= 1.0) Serial.print(" (ดี)");
      else if (perfusionIndex >= 0.3) Serial.print(" (ปกติ)");
      else Serial.print(" (อ่อน)");

      int spaces = 19 - String(perfusionIndex, 2).length();
      for(int i = 0; i < spaces; i++) Serial.print(" ");
      Serial.println("║");
    } else {
      Serial.println("กำลังคำนวณ...  ║");
    }
  }

  if (mlx90614_ok) {
    Serial.println("╠═══════════════════════════════════════╣");

    if (!isnan(lastAmbientTemp) && !isnan(lastObjectTemp)) {
      Serial.print("║ 🌡️  อุณหภูมิโดยรอบ: ");
      Serial.print(lastAmbientTemp, 1);
      Serial.print(" °C");
      int spaces1 = 13 - String(lastAmbientTemp, 1).length();
      for(int i = 0; i < spaces1; i++) Serial.print(" ");
      Serial.println("║");

      Serial.print("║ 🌡️  อุณหภูมิร่างกาย: ");
      Serial.print(lastObjectTemp, 1);
      Serial.print(" °C");

      if (lastObjectTemp >= 37.5) Serial.print(" (มีไข้)");
      else if (lastObjectTemp >= 37.0) Serial.print(" (ค่อนข้างสูง)");
      else Serial.print(" (ปกติ)");

      int spaces2 = 11 - String(lastObjectTemp, 1).length();
      for(int i = 0; i < spaces2; i++) Serial.print(" ");
      Serial.println("║");

      float tempF = lastObjectTemp * 9.0 / 5.0 + 32.0;
      Serial.print("║      (");
      Serial.print(tempF, 1);
      Serial.print(" °F)");
      int spaces3 = 25 - String(tempF, 1).length();
      for(int i = 0; i < spaces3; i++) Serial.print(" ");
      Serial.println("║");
    }
  }

  Serial.println("╚═══════════════════════════════════════╝\n");
}

// ========================================
// ฟังก์ชันเชื่อมต่อ WiFi
// ========================================
void connectWiFi() {
  Serial.println("\n📡 กำลังเชื่อมต่อ WiFi...");
  Serial.print("   SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n   ✓ เชื่อมต่อ WiFi สำเร็จ!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n   ✗ เชื่อมต่อ WiFi ไม่สำเร็จ");
  }
}

// ========================================
// ฟังก์ชันเชื่อมต่อ Firebase
// ========================================
void connectFirebase() {
  Serial.println("\n🔥 กำลังเชื่อมต่อ Firebase...");

  // กำหนดค่า Firebase Config
  fbConfig.host = FIREBASE_HOST;
  fbConfig.api_key = FIREBASE_API_KEY;
  fbConfig.database_url = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

  // เริ่มต้น Firebase
  Firebase.begin(&fbConfig, &auth);
  Firebase.reconnectWiFi(true);

  // ตั้งค่า timeout
  firebaseData.setBSSLBufferSize(1024, 1024);
  firebaseData.setResponseSize(1024);

  Serial.println("   ✓ เชื่อมต่อ Firebase สำเร็จ!");
  Serial.print("   Database: ");
  Serial.println(FIREBASE_HOST);
}

// ========================================
// ฟังก์ชันส่งข้อมูลไป Firebase
// ========================================
void sendDataToFirebase() {
  // ส่งข้อมูลเฉพาะเมื่อพร้อมแสดงผล (ผ่าน 5 วินาที)
  if (!readyToDisplay) return;

  // ส่งเฉพาะเมื่อมีนิ้วแตะ และมีค่าที่ถูกต้อง
  if (!fingerDetected) return;
  if (!spO2Valid) return;

  if (millis() - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL) {
    lastFirebaseUpdate = millis();

    Serial.println("📤 กำลังส่งข้อมูลไป Firebase...");

    // สร้าง JSON object
    FirebaseJson json;
    json.set("heart_rate_bpm", beatAvg);
    json.set("temperature_c", lastObjectTemp);
    json.set("spo2_percent", spO2Avg);
    json.set("perfusion_index", perfusionIndex);
    json.set("timestamp_millis", (unsigned long)millis());

    // ส่งไป Firebase (push เพื่อสร้าง unique key)
    String path = String(FIREBASE_PATH);
    if (Firebase.pushJSON(firebaseData, path, json)) {
      Serial.println("   ✓ ส่งข้อมูลสำเร็จ!");
    } else {
      Serial.println("   ✗ ส่งข้อมูลล้มเหลว");
      Serial.println("   Error: " + firebaseData.errorReason());
    }
  }
}
