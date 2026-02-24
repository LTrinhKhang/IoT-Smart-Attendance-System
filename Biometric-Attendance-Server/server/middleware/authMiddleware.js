const jwt = require("jsonwebtoken");

// 🛡 Middleware kiểm tra token JWT
exports.verifyToken = (req, res, next) => {
    const token = req.headers.authorization;

    if (!token) {
        return res.status(403).json({ message: "Không có token, từ chối truy cập!" });
    }

    try {
        const decoded = jwt.verify(token.split(" ")[1], process.env.JWT_SECRET);
        req.user = decoded; // Lưu thông tin user vào request
        next(); // Chuyển sang bước tiếp theo (controller)
    } catch (error) {
        return res.status(401).json({ message: "Token không hợp lệ!" });
    }
};

// 🛡 Middleware kiểm tra quyền Admin
exports.verifyAdmin = (req, res, next) => {
    if (!req.user || req.user.role !== "admin") {
        return res.status(403).json({ message: "Chỉ Admin mới có quyền truy cập!" });
    }
    next();
};

/* 
verifyToken(req, res, next): Kiểm tra token, nếu hợp lệ thì lưu thông tin vào req.user.
verifyAdmin(req, res, next): Chỉ Admin mới có thể tiếp tục request.
*/