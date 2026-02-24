const bcrypt = require("bcryptjs");
const jwt = require("jsonwebtoken");

// 📌 Mã hóa mật khẩu
exports.hashPassword = async (password) => {
    return await bcrypt.hash(password, 10);
};

// 📌 Kiểm tra mật khẩu nhập vào có khớp với mật khẩu đã mã hóa không
exports.verifyPassword = async (password, hashedPassword) => {
    return await bcrypt.compare(password, hashedPassword);
};

// 📌 Tạo token JWT
exports.generateToken = (userId, role) => {
    return jwt.sign({ userId, role }, process.env.JWT_SECRET, { expiresIn: "1h" });
};

// 📌 Chuẩn hóa chuỗi (viết hoa chữ cái đầu)
exports.capitalize = (str) => {
    return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
};

// 📌 Kiểm tra email hợp lệ
exports.validateEmail = (email) => {
    return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
};
