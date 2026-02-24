// Middleware bắt lỗi toàn bộ hệ thống
exports.errorHandler = (err, req, res, next) => {
    console.error("Lỗi:", err.message);
    res.status(err.status || 500).json({ message: err.message || "Lỗi server!" });
};
