// --- 1. FIREBASE CONFIGURATION & REFERENCES ---
const db = firebase.database();
// Path สำหรับอ่านข้อมูลสุขภาพ (ต้องตรงกับ ESP32: /health_data)
const healthDataRef = db.ref('health_data'); 
// 💡 NEW: Path สำหรับควบคุมไฟ Neon (ต้องตรงกับ ESP32: /light_control/command)
const lightCommandRef = db.ref('light_control/command'); 

// --- 2. UI ELEMENTS (ดึงองค์ประกอบจาก HTML) ---
const heartRateDisplay = document.getElementById('heartrate-display');
const tempDisplay = document.getElementById('temp-display');
const spo2Display = document.getElementById('spo2-display'); 
const timestampOutput = document.getElementById('timestamp-output'); 
const userEmailDisplay = document.getElementById('user-email-display');
const navButtons = document.querySelectorAll('.nav-button');
const contentTabs = document.querySelectorAll('.content-tab');
// 💡 NEW: เพิ่ม Light Status Display
const lightStatusDisplay = document.getElementById('light-status');


// ===========================================
// FUNCTION A: REALTIME HEALTH DATA LISTENER
// ===========================================
function listenForHealthData() {
    healthDataRef.limitToLast(1).on('value', (snapshot) => {
        if (snapshot.exists()) {
            snapshot.forEach((childSnapshot) => {
                const data = childSnapshot.val();
                
                const hr = data.heart_rate_bpm || '--';
                const temp = data.temperature_c ? parseFloat(data.temperature_c).toFixed(1) : '--'; 
                const spo2 = data.spo2_percent || '--';

                heartRateDisplay.innerText = hr;
                tempDisplay.innerText = temp;
                spo2Display.innerText = spo2;

                const timestamp = new Date(parseInt(data.timestamp_millis, 10));
                timestampOutput.innerText = `Last Updated: ${timestamp.toLocaleTimeString()}`;
            });
        } else {
            heartRateDisplay.innerText = 'No Data';
            tempDisplay.innerText = 'No Data';
            spo2Display.innerText = 'No Data';
            timestampOutput.innerText = 'No data available from ESP32.';
        }
    });
}


// ===========================================
// 💡 NEW: FUNCTION B: NEON LIGHT CONTROL SENDER (ส่งคำสั่งไปยัง ESP32)
// ===========================================
function sendLightCommand(command) {
    // 1. ส่งค่าคำสั่งไปยัง Firebase (ESP32 จะอ่านค่านี้)
    lightCommandRef.set(command)
        .then(() => {
            console.log(`Command sent: ${command}`);
            // 💡 NEW: แสดงสถานะการส่งสำเร็จ
            lightStatusDisplay.innerText = `Status: Command sent: ${command}`;
        })
        .catch((error) => {
            console.error("Firebase SET failed:", error);
            lightStatusDisplay.innerText = `Error sending command: ${error.message}`;
        });
}


// ===========================================
// FUNCTION C: INITIALIZATION
// ===========================================
function setupNavigation() {
    navButtons.forEach(button => {
        button.addEventListener('click', () => {
            const targetId = button.getAttribute('data-target');
            
            navButtons.forEach(btn => btn.classList.remove('active'));
            button.classList.add('active');

            contentTabs.forEach(tab => {
                if (tab.id === targetId) {
                    tab.classList.add('active');
                } else {
                    tab.classList.remove('active');
                }
            });
        });
    });
}

document.addEventListener('DOMContentLoaded', () => {
    setupNavigation();
    
    listenForHealthData(); 

    // โค้ดส่วนอื่นๆ (Auth, Logout)
    firebase.auth().onAuthStateChanged((user) => {
        if (user) {
            userEmailDisplay.innerText = user.email || 'User Logged In';
        } else {
            userEmailDisplay.innerText = 'Guest (Log in to sync)';
        }
    });

    document.getElementById('logout-button').addEventListener('click', () => {
        firebase.auth().signOut().then(() => {
            console.log("User signed out.");
            alert("Signed out successfully.");
        });
    });
    
    // 💡 NEW: เชื่อมต่อปุ่มกับฟังก์ชันส่งคำสั่ง
    document.querySelectorAll('.light-color-button').forEach(button => {
        button.addEventListener('click', () => {
            const color = button.getAttribute('data-color');
            console.log(`Sending command to set light color to: ${color}`);
            
            sendLightCommand(color); // 🚨 เรียกใช้ฟังก์ชันส่งคำสั่ง
            
            document.getElementById('light-status').innerText = `Sending command: ${color}...`;
        });
    });
});