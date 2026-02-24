const express = require("express");
const router = express.Router();
const db = require("../config/db");

// 🗓️ Hàm lấy thứ của hôm nay (Monday, Tuesday, ...)
function getToday() {
    const days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    return days[new Date().getDay()];
}

// 🚀 API tự động lấy danh sách lớp hôm nay
router.get("/", async (req, res) => {
    try {
        const today = getToday(); // Lấy thứ hôm nay
        console.log(` Hôm nay là: ${today}`);

        // 🛠️ Lấy tất cả các bảng trong database
        const [tables] = await db.query("SHOW TABLES;");
        const tableNames = tables.map(row => Object.values(row)[0]); // Lấy tên bảng

        let todayClasses = [];

        // 🔍 Kiểm tra từng bảng có dữ liệu của hôm nay không
        for (let tableName of tableNames) {
            if (!tableName.startsWith("class_")) continue; // Bỏ qua bảng không phải lớp học

            const query = `SELECT DISTINCT class_name, start_time, end_time FROM ${tableName} WHERE class_day = ?;`;
            const [rows] = await db.query(query, [today]);

            if (rows.length > 0) {
                todayClasses.push(...rows); // Thêm vào danh sách lớp hôm nay
            }
        }

        console.log(" Danh sách lớp hôm nay:", todayClasses);

        res.json({ success: true, classes: todayClasses });
    } catch (error) {
        console.error(" Lỗi API /classes/today:", error);
        res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
    }
});

module.exports = router;
