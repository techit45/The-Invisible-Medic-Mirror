// Element สำหรับฟอร์มและส่วนแสดงผล (ตรวจสอบว่า ID ตรงกันกับ index.html)
const loginForm = document.getElementById('login-form');
const registerForm = document.getElementById('register-form');

const loginSection = document.getElementById('login-section');
const registerSection = document.getElementById('register-section');
const showLoginLink = document.getElementById('show-login');
const showRegisterLink = document.getElementById('show-register');


// ===========================================
// 1. Logic การสลับฟอร์ม (แก้ไขปัญหาการเปลี่ยนหน้า)
// ===========================================

// สลับไปหน้า Register (เมื่อคลิก "Register" ในหน้า Log In)
showRegisterLink.addEventListener('click', (e) => {
    e.preventDefault(); // ป้องกันการเปลี่ยนหน้าตามลิงก์
    
    // ตรวจสอบให้แน่ใจว่า Elements ถูกหาเจอ ก่อนที่จะพยายามเปลี่ยน display style
    if (loginSection && registerSection) { 
        loginSection.style.display = 'none';
        registerSection.style.display = 'block';
        console.log("Switched to Register form.");
    } else {
        console.error("Error: Could not find login or register section elements.");
    }
});

// สลับไปหน้า Log In (เมื่อคลิก "Log In" ในหน้า Register)
showLoginLink.addEventListener('click', (e) => {
    e.preventDefault();
    if (loginSection && registerSection) {
        registerSection.style.display = 'none';
        loginSection.style.display = 'block';
        console.log("Switched to Log In form.");
    }
});


// ===========================================
// 2. Register Handler (การลงทะเบียน)
// ===========================================
registerForm.addEventListener('submit', (e) => {
    e.preventDefault();
    const email = document.getElementById('register-email').value;
    const password = document.getElementById('register-password').value;

    auth.createUserWithEmailAndPassword(email, password)
        .then(() => {
            alert("Registration successful! Redirecting to Dashboard.");
            window.location.href = 'dashboard.html'; 
        })
        .catch((error) => {
            alert("Registration failed: " + error.message);
        });
});


// ===========================================
// 3. Log In Handler (พร้อม Persistence/จดจำสถานะ)
// ===========================================
loginForm.addEventListener('submit', (e) => {
    e.preventDefault();
    const email = document.getElementById('login-email').value;
    const password = document.getElementById('login-password').value;

    // กำหนด Persistence เป็น LOCAL เพื่อจดจำสถานะถาวร
    auth.setPersistence(firebase.auth.Auth.Persistence.LOCAL) 
        .then(() => {
            return auth.signInWithEmailAndPassword(email, password);
        })
        .then(() => {
            alert("Login successful! Redirecting to Dashboard.");
            window.location.href = 'dashboard.html'; 
        })
        .catch((error) => {
            alert("Login failed: " + error.message);
        });
});