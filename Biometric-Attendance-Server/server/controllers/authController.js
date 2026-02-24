const { hashPassword, verifyPassword, generateToken, validateEmail } = require("../utils/helper");
const db = require("../config/db");

// Xử lý đăng ký người dùng
exports.register = async (req, res) => {
    const { username, email, password } = req.body;

    if (!validateEmail(email)) {
        return res.status(400).json({ message: "Email không hợp lệ!" });
    }

    try {
        //  Sửa lỗi truy vấn: Đảm bảo `existingUser` là một mảng
        const [existingUser] = await db.query("SELECT * FROM users WHERE email = ?", [email]);
        if (existingUser && existingUser.length > 0) {
            return res.status(400).json({ message: "Email đã được đăng ký!" });
        }

        const hashedPassword = await hashPassword(password);
        await db.query("INSERT INTO users (username, email, password) VALUES (?, ?, ?)", [username, email, hashedPassword]);

        res.status(201).json({ message: "Đăng ký thành công!" });
    } catch (error) {
        console.error(" Lỗi khi đăng ký:", error);
        res.status(500).json({ message: "Lỗi server!" });
    }
};
        
//  Xử lý đăng nhập
    exports.login = async (req, res) => {
        const { email, password } = req.body;
        
        try {
            const [users] = await db.query("SELECT * FROM users WHERE email = ?", [email]);
            if (!users || users.length === 0) {
                return res.status(401).json({ message: "Email hoặc mật khẩu không đúng!" });
            }
    
            const user = users[0];
            const isMatch = await verifyPassword(password, user.password);
            if (!isMatch) {
            return res.status(401).json({ message: "Email hoặc mật khẩu không đúng!" });
        }
        
        //  Tạo JWT token
        const token = generateToken(user.id, user.role);
        res.json({ message: "Đăng nhập thành công!", token });
        } catch (error) {
            console.error(" Lỗi khi đăng nhập:", error);
            res.status(500).json({ message: "Lỗi server!" });
        }
    };