const API_BASE = `http://${window.location.hostname}:5500`;

document.addEventListener("DOMContentLoaded", function () {
    console.log("Token trong localStorage:", localStorage.getItem("token"));
    localStorage.removeItem("token");

    const wrapper = document.querySelector('.wrapper');
    const registerLink = document.querySelector('.register-link');
    const loginLink = document.querySelector('.login-link');

    if (!registerLink || !loginLink || !wrapper) {
        console.error(" Không tìm thấy phần tử cần thiết trong DOM!");
        return;
    }

    registerLink.onclick = () => {
        wrapper.classList.add('active');
    };

    loginLink.onclick = () => {
        wrapper.classList.remove('active');
    };

    // Kiểm tra nếu chưa có token (tắt web thì mất token)
    if (!sessionStorage.getItem("token") && !window.location.pathname.includes("login.html")) {
        window.location.href = "login.html";
    }
});

// ĐĂNG NHẬP (LOGIN)
async function adminLogin(event) {
    event.preventDefault();

    const email = document.getElementById("adminEmail").value;
    const password = document.getElementById("adminPassword").value;

    try {
        const response = await fetch(`${API_BASE}/api/auth/login`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ email, password }),
        });

        const data = await response.json();

        if (!response.ok) throw new Error(data.message);

        // Lưu trạng thái đăng nhập
        sessionStorage.setItem("token", data.token);
        sessionStorage.setItem("isLoggedIn", "true");

        document.getElementById("loginMessage").innerText = " Đăng nhập thành công! Đang chuyển hướng...";
        setTimeout(() => {
            window.location.href = "index.html";
        }, 1000);
    } catch (error) {
        document.getElementById("loginMessage").style.color = "red";
        document.getElementById("loginMessage").innerText = ` ${error.message}`;
    }
}

// ĐĂNG KÝ (SIGNUP)
async function adminSignup(event) {
    event.preventDefault();

    const username = document.getElementById("signupUsername").value;
    const email = document.getElementById("signupEmail").value;
    const password = document.getElementById("signupPassword").value;
    const signupMessage = document.getElementById("signupMessage");

    try {
        const response = await fetch(`${API_BASE}/api/auth/register`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ username, email, password })
        });

        const data = await response.json();

        if (!response.ok) throw new Error(data.message);

        signupMessage.style.color = "green";
        signupMessage.innerText = " Đăng ký thành công! Vui lòng đăng nhập.";

        setTimeout(() => {
            document.querySelector('.wrapper').classList.remove('active');
            signupMessage.innerText = "";
        }, 2000);

    } catch (error) {
        signupMessage.style.color = "red";
        signupMessage.innerText = ` ${error.message}`;
    }
}

//  KIỂM TRA ĐĂNG NHẬP
function checkAuth() {
    if (!sessionStorage.getItem("token")) {
        window.location.href = "login.html";
    }
}

//  CHUYỂN HƯỚNG NẾU CHƯA ĐĂNG NHẬP
window.onload = function () {
    const isLoggedIn = sessionStorage.getItem("isLoggedIn");
    if (!isLoggedIn && !window.location.pathname.includes("login.html")) {
        window.location.href = "login.html";
    }
};

//  ĐĂNG XUẤT
function adminLogout() {
    sessionStorage.removeItem("token");
    sessionStorage.removeItem("isLoggedIn");
    alert("🚪 Bạn đã đăng xuất!");
    window.location.href = "login.html";
}

//  Gán các hàm vào `window` để gọi từ HTML
window.adminLogin = adminLogin;
window.adminSignup = adminSignup;
window.adminLogout = adminLogout;
window.checkAuth = checkAuth;