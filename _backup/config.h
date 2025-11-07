// config.h

// 1. ข้อมูล Wi-Fi
#define WIFI_SSID "IRE_TUTOR_TRUE_2.4G"
#define WIFI_PASSWORD "5687862020"

// 2. ข้อมูล Firebase (Realtime Database)
// ⚠️ ใช้ Database URL ที่ถูกต้อง (ไม่ใช่ authDomain)
#define FIREBASE_HOST "invisiblemedicdata-default-rtdb.asia-southeast1.firebasedatabase.app"

// API Key จาก Firebase Console
#define FIREBASE_API_KEY "AIzaSyDB2ULt3R58eBjYvztpfDK3v7hm6-RThhI"

// Database Secret (Legacy token) - ใช้สำหรับ ESP32
#define FIREBASE_AUTH "Dc2Y7eRpLX0ujAWbxn0rLzmH7A1QEShVCEnAw1kD"

// 3. กำหนด Path (โฟลเดอร์ในฐานข้อมูล) ที่จะใช้เก็บข้อมูล
#define FIREBASE_PATH "/health_data" // เปลี่ยนเป็น path ใหม่ที่ชัดเจน