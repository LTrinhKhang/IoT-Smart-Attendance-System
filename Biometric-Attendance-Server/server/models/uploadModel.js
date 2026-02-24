const db = require("../config/db");

const UploadModel = {
    async createTable(className) {
        const tableName = `class_${className.replace(/\W+/g, "_")}`;
        console.log(`🔧 Đang tạo bảng: ${tableName}`);

        const sql = `
            CREATE TABLE IF NOT EXISTS ${tableName} (
                id INT AUTO_INCREMENT PRIMARY KEY,
                student_id VARCHAR(50) NOT NULL,
                name VARCHAR(255) NOT NULL,
                class_name VARCHAR(255) NOT NULL,
                class_day VARCHAR(20) NOT NULL,
                start_time TIME NOT NULL,
                end_time TIME NOT NULL,
                UNIQUE KEY unique_student (student_id)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        `;

        try {
            await db.query(sql);
            console.log(` Bảng ${tableName} đã được tạo hoặc đã tồn tại.`);
        } catch (error) {
            console.error(` Lỗi khi tạo bảng ${tableName}:`, error);
        }
    },

    //  Thêm hàm insertStudents
    async insertStudents(className, students) {
        const tableName = `class_${className.replace(/\W+/g, "_")}`;
        console.log(` Đang chèn dữ liệu vào bảng ${tableName}`);

        const sql = `INSERT INTO ${tableName} (student_id, name, class_name, class_day, start_time, end_time) VALUES ?`;

        const values = students.map(s => [s.student_id, s.name,className, s.class_day, s.start_time, s.end_time]);

        try {
            await db.query(sql, [values]);
            console.log(` Đã thêm ${students.length} sinh viên vào ${tableName}`);
        } catch (error) {
            console.error(` Lỗi khi thêm sinh viên vào bảng ${tableName}:`, error);
        }
    }
};

//  Đảm bảo cả hai hàm được export
module.exports = UploadModel;
