const db = require("../config/db");

const setupTables = {
    async ensureFaceAttendanceTable() {
        const sql = `
            CREATE TABLE IF NOT EXISTS face_attendance (
                id INT AUTO_INCREMENT PRIMARY KEY,
                student_id VARCHAR(50) NOT NULL,
                student_name VARCHAR(100) NOT NULL,
                class_name VARCHAR(100) NOT NULL,
                attendance_date DATE NOT NULL,
                attendance_time TIME NOT NULL,
                confidence FLOAT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        `;

        try {
            await db.query(sql);
            console.log("✅ Bảng 'face_attendance' đã được tạo hoặc đã tồn tại.");
        } catch (err) {
            console.error("❌ Lỗi khi tạo bảng 'face_attendance':", err);
        }
    }
};

module.exports = setupTables;
