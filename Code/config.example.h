// config.example.h
// Copy this file to config.h and fill in your actual credentials

// 1. ข้อมูล Wi-Fi
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// 2. ข้อมูล Firebase (Realtime Database)
// ⚠️ ใช้ Database URL ที่ถูกต้อง (ไม่ใช่ authDomain)
#define FIREBASE_HOST "your-database.firebaseio.com"

// API Key จาก Firebase Console
#define FIREBASE_API_KEY "YOUR_FIREBASE_API_KEY"

// Database Secret (Legacy token) - ใช้สำหรับ ESP32
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRET"

// 3. กำหนด Path (โฟลเดอร์ในฐานข้อมูล) ที่จะใช้เก็บข้อมูล
#define FIREBASE_PATH "/health_data"
