const express = require('express');
const moment = require('moment'); // Chỉ khai báo 1 lần trong file này
const db = require('../config/db'); // Import kết nối database

const router = express.Router();

router.get("/class-schedule", async (req, res) => {
    try {
        const [tables] = await db.query("SHOW TABLES LIKE 'class_%'");
        if (tables.length === 0) {
            return res.json([]); // Nếu không có bảng nào, trả về mảng rỗng
        }

        let events = []; // Dùng mảng để FullCalendar hiển thị lặp lại

        for (let table of tables) {
            const tableName = Object.values(table)[0]; // Lấy tên bảng
            const [data] = await db.query(`
                SELECT DISTINCT class_name, class_day, start_time, end_time 
                FROM ${tableName}
            `);

            const dayMapping = {
                "Monday": 1, "Tuesday": 2, "Wednesday": 3,
                "Thursday": 4, "Friday": 5, "Saturday": 6, "Sunday": 0
            };

            data.forEach(row => {
                let today = moment();
                let firstValidDate = today.startOf('month').day(dayMapping[row.class_day]);

                if (firstValidDate.isBefore(today.startOf('month'))) {
                    firstValidDate.add(7, 'days');
                }

                events.push({
                    title: row.class_name,
                    startTime: row.start_time,
                    endTime: row.end_time,
                    startRecur: firstValidDate.format('YYYY-MM-DD'),
                    daysOfWeek: [dayMapping[row.class_day]], // Lặp lại vào thứ cố định mỗi tuần
                    color: "#4287f5"
                });
            });
        }

        res.json(events);
    } catch (error) {
        console.error(" Lỗi lấy lịch học:", error);
        res.status(500).json({ message: "Lỗi server!" });
    }
});

module.exports = router;
