// controllers/faceSignalController.js

let pendingMSSV = null;
let attendanceSignal = false; // ✅ Thêm: Biến toàn cục lưu tín hiệu điểm danh

// Nhận tín hiệu đăng ký từ ESP32
exports.receiveRegisterSignal = (req, res) => {
    const { mssv } = req.body;

    if (!mssv) {
        return res.status(400).json({
            success: false,
            message: 'Thiếu MSSV'
        });
    }

    console.log("📥 ESP32 yêu cầu đăng ký khuôn mặt cho MSSV:", mssv);
    pendingMSSV = mssv;

    return res.status(200).json({
        success: true,
        message: 'Đã nhận tín hiệu đăng ký',
        mssv: mssv
    });
};

// Gửi MSSV chờ đăng ký xuống frontend
exports.getPendingRegisterSignal = (req, res) => {
    if (pendingMSSV) {
        const mssvToSend = pendingMSSV;
        pendingMSSV = null;

        return res.status(200).json({
            success: true,
            mssv: mssvToSend
        });
    } else {
        return res.status(200).json({
            success: false,
            message: 'Không có MSSV chờ đăng ký'
        });
    }
};

// ✅ Thêm: Nhận tín hiệu điểm danh (ESP32 có thể gọi POST này)
exports.receiveAttendanceSignal = (req, res) => {
    attendanceSignal = true;
    console.log("📥 ESP32 gửi tín hiệu điểm danh!");
    return res.status(200).json({ success: true, message: "Đã bật tín hiệu điểm danh" });
};

// ✅ Thêm: Frontend sẽ poll GET liên tục route này để biết có tín hiệu không
exports.getAttendanceSignal = (req, res) => {
    if (attendanceSignal) {
        // Sau khi frontend nhận, reset lại
        attendanceSignal = false;
        return res.status(200).json({ success: true, signal: true });
    } else {
        return res.status(200).json({ success: true, signal: false });
    }
};
