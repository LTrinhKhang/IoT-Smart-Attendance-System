const db = require('../config/db');

const FaceRecognitionModel = {
    async initialize() {
        // Tạo bảng face_descriptors
        const faceDescriptorsSql = `
            CREATE TABLE IF NOT EXISTS face_descriptors (
                id VARCHAR(50) PRIMARY KEY,
                name VARCHAR(100) NOT NULL,
                class VARCHAR(100) NOT NULL,
                descriptor TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        `;
        await db.query(faceDescriptorsSql);

        // Tạo bảng face_attendance
        const faceAttendanceSql = `
            CREATE TABLE IF NOT EXISTS face_attendance (
                id INT AUTO_INCREMENT PRIMARY KEY,
                student_id VARCHAR(50) NOT NULL,
                student_name VARCHAR(100) NOT NULL,
                class_name VARCHAR(100) NOT NULL,
                attendance_date DATE NOT NULL,
                attendance_time TIME NOT NULL,
                confidence FLOAT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE KEY unique_attendance (student_id, attendance_date)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        `;
        await db.query(faceAttendanceSql);

        console.log('✅ Face Recognition tables initialized successfully');

        const fingerprintSql = `
        CREATE TABLE IF NOT EXISTS fingerprint_templates (
            id VARCHAR(50) PRIMARY KEY,
            template_base64 LONGTEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        `;
        await db.query(fingerprintSql);

    },

    // ===== FACE DESCRIPTORS METHODS =====
    async registerFace(data) {
        const sql = `
            INSERT INTO face_descriptors (id, name, class, descriptor) 
            VALUES (?, ?, ?, ?)
            ON DUPLICATE KEY UPDATE 
            name = VALUES(name), 
            class = VALUES(class), 
            descriptor = VALUES(descriptor),
            updated_at = CURRENT_TIMESTAMP
        `;
        const values = [
            data.id,
            data.name,
            data.class,
            JSON.stringify(data.descriptor)
        ];

        const [result] = await db.query(sql, values);
        return {
            id: data.id,
            name: data.name,
            class: data.class,
            affectedRows: result.affectedRows
        };
    },

    async getAllFaceDescriptors() {
        const [rows] = await db.query("SELECT id, name, class, descriptor, created_at FROM face_descriptors ORDER BY created_at DESC");
        return rows;
    },

    async getFaceDescriptorByStudentId(studentId) {
        const [rows] = await db.query("SELECT * FROM face_descriptors WHERE id = ?", [studentId]);
        return rows[0] || null;
    },

    async deleteFaceDescriptor(studentId) {
        const [result] = await db.query("DELETE FROM face_descriptors WHERE id = ?", [studentId]);
        
        if (result.affectedRows > 0) {
            return { success: true, message: 'Face descriptor deleted successfully' };
        } else {
            return { success: false, message: 'Face descriptor not found' };
        }
    },
    async saveFingerprintTemplate(id, base64Data) {
    const sql = `
        INSERT INTO fingerprint_templates (id, template_base64)
        VALUES (?, ?)
        ON DUPLICATE KEY UPDATE
        template_base64 = VALUES(template_base64),
        updated_at = CURRENT_TIMESTAMP
    `;
    const values = [id, base64Data];
    await db.query(sql, values);
    },

    // ===== ATTENDANCE METHODS =====
    async recordAttendance(data) {
        const now = new Date();
        const attendanceDate = now.toISOString().split('T')[0]; // YYYY-MM-DD
        const attendanceTime = now.toTimeString().split(' ')[0]; // HH:MM:SS

        const sql = `
            INSERT INTO face_attendance 
            (student_id, student_name, class_name, attendance_date, attendance_time, confidence) 
            VALUES (?, ?, ?, ?, ?, ?)
        `;
        const values = [
            data.studentInfo.id,
            data.studentInfo.name,
            data.className,
            attendanceDate,
            attendanceTime,
            data.confidence
        ];

        const [result] = await db.query(sql, values);
        return {
            id: result.insertId,
            student_id: data.studentInfo.id,
            student_name: data.studentInfo.name,
            class_name: data.className,
            attendance_date: attendanceDate,
            attendance_time: attendanceTime,
            confidence: data.confidence
        };
    },

    async getAttendanceRecords(filters = {}) {
        let sql = `
            SELECT 
                fa.*,
                DATE_FORMAT(fa.attendance_time, '%H:%i') as formatted_time,
                DATE_FORMAT(fa.attendance_date, '%d/%m/%Y') as formatted_date
            FROM face_attendance fa 
            WHERE 1=1
        `;
        const values = [];

        if (filters.studentId) {
            sql += ' AND fa.student_id = ?';
            values.push(filters.studentId);
        }

        if (filters.className) {
            sql += ' AND fa.class_name = ?';
            values.push(filters.className);
        }

        if (filters.date) {
            sql += ' AND fa.attendance_date = ?';
            values.push(filters.date);
        }

        if (filters.dateFrom) {
            sql += ' AND fa.attendance_date >= ?';
            values.push(filters.dateFrom);
        }

        if (filters.dateTo) {
            sql += ' AND fa.attendance_date <= ?';
            values.push(filters.dateTo);
        }

        sql += ' ORDER BY fa.attendance_date DESC, fa.attendance_time DESC';

        if (filters.limit) {
            sql += ' LIMIT ?';
            values.push(parseInt(filters.limit));
        }

        const [rows] = await db.query(sql, values);
        return rows;
    },

    async getAttendanceStats(filters = {}) {
        let sql = `
            SELECT 
                COUNT(*) as total_records,
                COUNT(DISTINCT student_id) as unique_students,
                COUNT(DISTINCT class_name) as unique_classes,
                AVG(confidence) as avg_confidence,
                SUM(CASE WHEN attendance_date = CURDATE() THEN 1 ELSE 0 END) as today_records
            FROM face_attendance 
            WHERE 1=1
        `;
        const values = [];

        if (filters.dateFrom) {
            sql += ' AND attendance_date >= ?';
            values.push(filters.dateFrom);
        }

        if (filters.dateTo) {
            sql += ' AND attendance_date <= ?';
            values.push(filters.dateTo);
        }

        if (filters.className) {
            sql += ' AND class_name = ?';
            values.push(filters.className);
        }

        const [rows] = await db.query(sql, values);
        return rows[0];
    },

    async getTodayAttendance() {
        const sql = `
            SELECT 
                fa.*,
                DATE_FORMAT(fa.attendance_time, '%H:%i') as formatted_time
            FROM face_attendance fa
            WHERE fa.attendance_date = CURDATE()
            ORDER BY fa.attendance_time DESC
        `;
        const [rows] = await db.query(sql);
        return rows;
    },

    async getClassesWithFaceData() {
        const sql = `
            SELECT 
                class_name,
                COUNT(*) as student_count,
                AVG(confidence) as avg_confidence
            FROM face_attendance 
            GROUP BY class_name 
            ORDER BY student_count DESC
        `;
        const [rows] = await db.query(sql);
        return rows;
    },

    async getStudentInfo(id) {
        const [tables] = await db.query("SHOW TABLES LIKE 'class_%'");
        for (const table of tables) {
            const tableName = Object.values(table)[0];
            const [rows] = await db.query(`SELECT * FROM ${tableName} WHERE student_id = ?`, [id]);
            if (rows.length > 0) return rows[0];
        }
        return null;
    }



};


module.exports = FaceRecognitionModel;
