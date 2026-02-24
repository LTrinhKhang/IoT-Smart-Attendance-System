require("dotenv").config();
const express = require('express');
const cors = require('cors');
const bodyParser = require("body-parser");
const db = require("./config/db");
const os = require('os');
const path = require('path');


const { errorHandler } = require("./middleware/errorHandler");
const setupTables = require('./utils/setupTables');


const app = express();

// Middleware
app.use(cors({ origin: '*' }));
app.use(express.json());
app.use(bodyParser.json());

// Phục vụ file tĩnh
const publicPath = path.join(__dirname, '../public');
app.use(express.static(publicPath));

// Routes
const API_ROUTES = {
    attendance: "/api/attendance",
    employees: "/api/employees",
    upload: "/api/upload",
    auth: "/api/auth"
};


const authRoutes = require("./routes/authRoutes");
const attendanceRoutes = require("./routes/attendanceRoutes");
const employeeRoutes = require("./routes/employeeRoutes");
const uploadRoutes = require("./routes/uploadRoutes");
const configRoutes = require("./routes/configRoutes");
const faceRecognitionRoutes = require('./routes/faceRecognitionRoutes');
const registrationRoutes = require('./routes/registrationRoutes');




app.use("/api/config", configRoutes);
app.use(API_ROUTES.auth, authRoutes);
app.use(API_ROUTES.attendance, attendanceRoutes);
app.use(API_ROUTES.employees, employeeRoutes);
app.use(API_ROUTES.upload, uploadRoutes);
app.use('/api/face-recognition', faceRecognitionRoutes);
app.use('/api', registrationRoutes);




const moment = require('moment'); // Thêm thư viện moment.js


(async () => {
  await setupTables.ensureFaceAttendanceTable();
})();

app.get("/api/class-schedule", async (req, res) => {
    try {
        // Lấy danh sách tất cả các bảng lớp học
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

                // Nếu ngày đầu tiên bị trôi về tháng trước, lấy tuần tiếp theo
                if (firstValidDate.isBefore(today.startOf('month'))) {
                    firstValidDate.add(7, 'days');
                }

                // Lặp lại sự kiện hàng tuần
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
        console.error("Lỗi lấy lịch học:", error);
        res.status(500).json({ message: "Lỗi server!" });
    }
});


app.get("/api/attendance", async (req, res) => {
    console.log("API attendance được gọi");

    try {
        const today = getToday(); // Lấy thứ hôm nay
        console.log(` Hôm nay là: ${today}`);

        const [tables] = await db.query("SHOW TABLES;");
        const tableNames = tables.map(row => Object.values(row)[0]); // Lấy tên bảng

        let todayClasses = [];

        // Kiểm tra từng bảng có dữ liệu của hôm nay không
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
        console.error(" Lỗi khi lấy danh sách lớp:", error);
        res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
    }
});




const FaceRecognitionModel = require('./models/faceRecognitionModel');

// Kiểm tra kết nối database và khởi tạo bảng Face Recognition
db.getConnection()
    .then(async () => {
        console.log("Kết nối MySQL thành công!");
        
        // Khởi tạo bảng Face Recognition
        try {
            await FaceRecognitionModel.initialize();
            console.log("Face Recognition tables initialized successfully!");
        } catch (error) {
            console.error("Error initializing Face Recognition tables:", error);
        }

        const PORT = process.env.PORT || 5500;
        app.listen(PORT, "0.0.0.0", () => {
            console.log(` Server đang chạy tại http://${getLocalIP()}:${PORT}`);
            
        });
    })
    .catch((err) => {
        console.error("Không thể kết nối Database:", err);
        process.exit(1);
    });

// Hàm lấy IP cục bộ
function getLocalIP() {
    const networkInterfaces = os.networkInterfaces();
    for (const key in networkInterfaces) {
        for (const net of networkInterfaces[key]) {
            if (net.family === 'IPv4' && !net.internal) {
                return net.address;
            }
        }
    }
    return '127.0.0.1';
}

// Hàm lấy thứ hôm nay
function getToday() {
    const days = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
    const today = new Date();
    return days[today.getDay()];
}

// Middleware xử lý lỗi cuối cùng
app.use(errorHandler);