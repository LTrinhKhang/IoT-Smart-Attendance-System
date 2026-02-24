const db = require('../config/db');
const fs = require('fs');
const path = require('path');

// Đảm bảo thư mục tồn tại
const ensureDirectoryExists = (dirPath) => {
    if (!fs.existsSync(dirPath)) {
        fs.mkdirSync(dirPath, { recursive: true });
    }
};

const facesDir = path.join(__dirname, '../../public/faces');
const fingerprintsDir = path.join(__dirname, '../../public/fingerprints');
ensureDirectoryExists(facesDir);
ensureDirectoryExists(fingerprintsDir);

const registrationController = {
    // ✅ Kiểm tra trạng thái sinh viên
    async checkStudentStatus(req, res) {
        try {
            const { id } = req.query;
            if (!id) {
                return res.status(400).json({ success: false, message: 'Thiếu mã số sinh viên' });
            }

            // Kiểm tra sinh viên có trong bảng class_*
            const [tables] = await db.query("SHOW TABLES LIKE 'class_%'");
            let studentExists = false;
            let studentInfo = null;

            for (const table of tables) {
                const tableName = Object.values(table)[0];
                const [students] = await db.query(`SELECT * FROM ${tableName} WHERE student_id = ?`, [id]);
                if (students.length > 0) {
                    studentExists = true;
                    studentInfo = students[0];
                    break;
                }
            }

            if (!studentExists) {
                return res.status(404).json({ success: false, message: 'Không tìm thấy sinh viên' });
            }

            // Kiểm tra đã đăng ký khuôn mặt
            const [faceResults] = await db.query("SELECT * FROM face_descriptors WHERE id = ?", [id]);
            const hasFace = faceResults.length > 0;

            // Kiểm tra đã có file vân tay
            const fingerprintFile = path.join(fingerprintsDir, `${id}.dat`);
            const hasFingerprint = fs.existsSync(fingerprintFile);

            return res.json({
                success: true,
                studentId: id,
                studentName: studentInfo.name,
                className: studentInfo.class_name,
                hasFace,
                hasFingerprint
            });
        } catch (error) {
            console.error("❌ Lỗi kiểm tra sinh viên:", error);
            res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
        }
    },

    // ✅ Đăng ký khuôn mặt
    async registerFace(req, res) {
        try {
            const { id, image } = req.body;
            if (!id || !image) {
                return res.status(400).json({ success: false, message: 'Thiếu thông tin: id, image' });
            }

            // Tìm sinh viên
            const [tables] = await db.query("SHOW TABLES LIKE 'class_%'");
            let studentInfo = null;
            for (const table of tables) {
                const tableName = Object.values(table)[0];
                const [students] = await db.query(`SELECT * FROM ${tableName} WHERE student_id = ?`, [id]);
                if (students.length > 0) {
                    studentInfo = students[0];
                    break;
                }
            }

            if (!studentInfo) {
                return res.status(404).json({ success: false, message: 'Không tìm thấy sinh viên' });
            }

            // Lưu ảnh
            const base64Data = image.replace(/^data:image\/\w+;base64,/, '');
            const imagePath = path.join(facesDir, `${id}.jpg`);
            fs.writeFileSync(imagePath, base64Data, { encoding: 'base64' });

            // Placeholder descriptor
            const descriptor = Array(128).fill(0);

            const [existing] = await db.query("SELECT * FROM face_descriptors WHERE id = ?", [id]);
            if (existing.length > 0) {
                await db.query("UPDATE face_descriptors SET descriptor = ?, updated_at = NOW() WHERE id = ?", [JSON.stringify(descriptor), id]);
            } else {
                await db.query("INSERT INTO face_descriptors (id, name, class, descriptor) VALUES (?, ?, ?, ?)", [
                    id, studentInfo.name, studentInfo.class_name, JSON.stringify(descriptor)
                ]);
            }

            res.json({
                success: true,
                message: "Đăng ký khuôn mặt thành công",
                studentId: id,
                imagePath: `/faces/${id}.jpg`
            });
        } catch (error) {
            console.error("❌ Lỗi đăng ký khuôn mặt:", error);
            res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
        }
    },

    // ✅ Đăng ký vân tay → Lưu file + database
    async registerTemplate(req, res) {
        try {
            const { id, template } = req.body;
            if (!id || !template) {
                return res.status(400).json({ success: false, message: 'Thiếu thông tin: id, template' });
            }

            // Lưu file vân tay
            const templatePath = path.join(fingerprintsDir, `${id}.dat`);
            fs.writeFileSync(templatePath, template, { encoding: 'base64' });

            // Lưu vào DB
            await db.query(`
                INSERT INTO fingerprint_templates (id, template_base64)
                VALUES (?, ?)
                ON DUPLICATE KEY UPDATE 
                    template_base64 = VALUES(template_base64),
                    updated_at = CURRENT_TIMESTAMP
            `, [id, template]);

            res.status(200).json({
                success: true,
                message: 'Đăng ký vân tay thành công',
                studentId: id,
                templatePath: `/fingerprints/${id}.dat`,
                dbSaved: true
            });
        } catch (error) {
            console.error("❌ Lỗi đăng ký vân tay:", error);
            res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
        }
    }
};

module.exports = registrationController;
