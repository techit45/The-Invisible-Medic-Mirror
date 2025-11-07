# 🔧 Setup Guide - The Invisible Medic

## 📋 Prerequisites

Before running this project, you need to set up your API keys and credentials.

## 🔐 Security Notice

**IMPORTANT:** Never commit API keys or credentials to GitHub!

## 🚀 Setup Steps

### 1. Web Application Setup

#### Step 1.1: Create config.js
```bash
cp config.example.js config.js
```

#### Step 1.2: Edit config.js
Open `config.js` and replace with your actual credentials:

```javascript
// Firebase Configuration (get from Firebase Console)
const firebaseConfig = {
    apiKey: "YOUR_FIREBASE_API_KEY",
    authDomain: "your-project.firebaseapp.com",
    databaseURL: "https://your-database.firebaseio.com",
    projectId: "your-project-id",
    storageBucket: "your-project.appspot.com",
    messagingSenderId: "YOUR_SENDER_ID",
    appId: "YOUR_APP_ID"
};

// Gemini AI API Key (get from Google AI Studio)
const GEMINI_API_KEY = 'YOUR_GEMINI_API_KEY';
```

**Where to get these:**
- **Firebase**: https://console.firebase.google.com/
  1. Go to Project Settings > General
  2. Scroll down to "Your apps"
  3. Copy the config object

- **Gemini API**: https://makersuite.google.com/app/apikey
  1. Click "Get API Key"
  2. Create or select a project
  3. Copy the API key

### 2. Arduino/ESP32 Setup

#### Step 2.1: Create config.h
```bash
cd Code
cp config.example.h config.h
```

#### Step 2.2: Edit config.h
Open `Code/config.h` and add your credentials:

```cpp
// Wi-Fi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Configuration
#define FIREBASE_HOST "your-database.firebaseio.com"
#define FIREBASE_API_KEY "YOUR_FIREBASE_API_KEY"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"

// Data Path
#define FIREBASE_PATH "/health_data"
```

**Where to get these:**
- **WIFI_SSID/PASSWORD**: Your home/office WiFi credentials
- **FIREBASE_HOST**: From Firebase Console > Realtime Database
- **FIREBASE_API_KEY**: Same as web config above
- **FIREBASE_AUTH**: Firebase Console > Project Settings > Service Accounts > Database Secrets

### 3. Running the Project

#### Web Application
1. Open `index.html` in a web browser
2. Or deploy to Firebase Hosting:
   ```bash
   firebase deploy --only hosting
   ```

#### ESP32
1. Open `Code/Code.ino` in Arduino IDE
2. Install required libraries:
   - MAX30105
   - Adafruit_MLX90614
   - FirebaseESP32
3. Select board: ESP32 Dev Module
4. Upload the code

## 📁 File Structure

```
├── config.js                  # Your API keys (not in git)
├── config.example.js          # Template for config.js
├── Code/
│   ├── config.h              # Your WiFi & Firebase (not in git)
│   ├── config.example.h      # Template for config.h
│   └── Code.ino              # Arduino code
├── index.html                # User registration
├── ai-health-analysis.html   # Main dashboard
└── qr-code.html              # QR code generator
```

## 🔒 Security Best Practices

1. ✅ **Never** commit `config.js` or `Code/config.h` to git
2. ✅ Use `.gitignore` to exclude sensitive files
3. ✅ Rotate API keys if accidentally exposed
4. ✅ Use Firebase Security Rules to protect database
5. ✅ Enable Firebase Authentication for production

## 🆘 Troubleshooting

### "config.js not found"
- Make sure you copied `config.example.js` to `config.js`
- Check that `config.js` is in the root directory

### "Firebase: Error (auth/invalid-api-key)"
- Verify your API keys in `config.js`
- Make sure there are no extra spaces or quotes

### ESP32 won't connect
- Check WiFi credentials in `Code/config.h`
- Verify Firebase database URL is correct
- Ensure ESP32 board is selected in Arduino IDE

## 📞 Support

If you need help, please open an issue on GitHub (without including any API keys!)

---

**🤖 Happy Coding!**
