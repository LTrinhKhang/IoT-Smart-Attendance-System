const FaceRecognitionModel = require('../models/faceRecognitionModel');
const { validationResult } = require('express-validator');

class FaceRecognitionController {
    // Register a new face descriptor
    static async registerFace(req, res) {
    try {
        const { id, descriptor } = req.body;

        if (!id || !Array.isArray(descriptor) || descriptor.length !== 128) {
            return res.status(400).json({
                success: false,
                message: 'Thiếu id hoặc descriptor không hợp lệ'
            });
        }

        // Truy tìm sinh viên từ bảng class_*
        const student = await FaceRecognitionModel.getStudentInfo(id);
        if (!student) {
            return res.status(404).json({
                success: false,
                message: 'Không tìm thấy sinh viên trong bảng lớp'
            });
        }

        // Ghi vào CSDL
        const result = await FaceRecognitionModel.registerFace({
            id,
            name: student.name,
            class: student.class_name,
            descriptor
        });

        res.status(201).json({
            success: true,
            message: 'Đăng ký khuôn mặt thành công',
            data: result
        });

    } catch (error) {
        console.error("❌ Lỗi trong registerFace:", error);
        res.status(500).json({
            success: false,
            message: 'Lỗi server khi đăng ký khuôn mặt'
        });
    }
}


    // Get all face descriptors
    static async getAllFaceDescriptors(req, res) {
        try {
            const faceDescriptors = await FaceRecognitionModel.getAllFaceDescriptors();

            res.json({
                success: true,
                message: 'Face descriptors retrieved successfully',
                data: faceDescriptors,
                count: faceDescriptors.length
            });

        } catch (error) {
            console.error('Error in getAllFaceDescriptors:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Get face descriptor by student ID
    static async getFaceDescriptorByStudentId(req, res) {
        try {
            const { studentId } = req.params;

            if (!studentId) {
                return res.status(400).json({
                    success: false,
                    message: 'Student ID is required'
                });
            }

            const faceDescriptor = await FaceRecognitionModel.getFaceDescriptorByStudentId(studentId);

            if (!faceDescriptor) {
                return res.status(404).json({
                    success: false,
                    message: 'Face descriptor not found for this student'
                });
            }

            res.json({
                success: true,
                message: 'Face descriptor retrieved successfully',
                data: faceDescriptor
            });

        } catch (error) {
            console.error('Error in getFaceDescriptorByStudentId:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Record face attendance
    static async recordAttendance(req, res) {
        try {
            // Check for validation errors
            const errors = validationResult(req);
            if (!errors.isEmpty()) {
                return res.status(400).json({
                    success: false,
                    message: 'Validation errors',
                    errors: errors.array()
                });
            }

            const { studentInfo, className, timestamp, confidence } = req.body;

            // Validate required fields
            if (!studentInfo || !className || !timestamp || confidence === undefined) {
                return res.status(400).json({
                    success: false,
                    message: 'Missing required fields: studentInfo, className, timestamp, confidence'
                });
            }

            // Validate confidence range
            if (confidence < 0 || confidence > 1) {
                return res.status(400).json({
                    success: false,
                    message: 'Confidence must be between 0 and 1'
                });
            }

            const result = await FaceRecognitionModel.recordAttendance({
                studentInfo,
                className,
                timestamp,
                confidence
            });

            res.status(201).json({
                success: true,
                message: 'Attendance recorded successfully',
                data: result
            });

        } catch (error) {
            console.error('Error in recordAttendance:', error);
            
            // Handle duplicate entry error
            if (error.code === 'ER_DUP_ENTRY') {
                return res.status(409).json({
                    success: false,
                    message: 'Attendance already recorded for this student today'
                });
            }

            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Get attendance records
    static async getAttendanceRecords(req, res) {
        try {
            const filters = {
                studentId: req.query.studentId,
                className: req.query.className,
                date: req.query.date,
                dateFrom: req.query.dateFrom,
                dateTo: req.query.dateTo,
                limit: req.query.limit
            };

            // Remove undefined filters
            Object.keys(filters).forEach(key => {
                if (filters[key] === undefined) {
                    delete filters[key];
                }
            });

            const attendanceRecords = await FaceRecognitionModel.getAttendanceRecords(filters);

            res.json({
                success: true,
                message: 'Attendance records retrieved successfully',
                data: attendanceRecords,
                count: attendanceRecords.length,
                filters: filters
            });

        } catch (error) {
            console.error('Error in getAttendanceRecords:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Get attendance statistics
    static async getAttendanceStats(req, res) {
        try {
            const filters = {
                dateFrom: req.query.dateFrom,
                dateTo: req.query.dateTo,
                className: req.query.className
            };

            // Remove undefined filters
            Object.keys(filters).forEach(key => {
                if (filters[key] === undefined) {
                    delete filters[key];
                }
            });

            const stats = await FaceRecognitionModel.getAttendanceStats(filters);

            // Format the response
            const formattedStats = {
                totalRecords: parseInt(stats.total_records) || 0,
                uniqueStudents: parseInt(stats.unique_students) || 0,
                uniqueClasses: parseInt(stats.unique_classes) || 0,
                averageConfidence: parseFloat(stats.avg_confidence) || 0,
                todayRecords: parseInt(stats.today_records) || 0
            };

            res.json({
                success: true,
                message: 'Attendance statistics retrieved successfully',
                data: formattedStats,
                filters: filters
            });

        } catch (error) {
            console.error('Error in getAttendanceStats:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Delete face descriptor
    static async deleteFaceDescriptor(req, res) {
        try {
            const { studentId } = req.params;

            if (!studentId) {
                return res.status(400).json({
                    success: false,
                    message: 'Student ID is required'
                });
            }

            const result = await FaceRecognitionModel.deleteFaceDescriptor(studentId);

            if (!result.success) {
                return res.status(404).json({
                    success: false,
                    message: result.message
                });
            }

            res.json({
                success: true,
                message: result.message
            });

        } catch (error) {
            console.error('Error in deleteFaceDescriptor:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Get classes with face recognition data
    static async getClassesWithFaceData(req, res) {
        try {
            const classes = await FaceRecognitionModel.getClassesWithFaceData();

            res.json({
                success: true,
                message: 'Classes with face data retrieved successfully',
                data: classes,
                count: classes.length
            });

        } catch (error) {
            console.error('Error in getClassesWithFaceData:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }

    // Get today's attendance summary
    static async getTodayAttendanceSummary(req, res) {
        try {
            const today = new Date().toISOString().split('T')[0];
            
            const attendanceRecords = await FaceRecognitionModel.getAttendanceRecords({
                date: today,
                limit: 100
            });
            

            const stats = await FaceRecognitionModel.getAttendanceStats({
                dateFrom: today,
                dateTo: today
            });

            // Group by class
            const classSummary = {};
            attendanceRecords.forEach(record => {
                if (!classSummary[record.class_name]) {
                    classSummary[record.class_name] = {
                        className: record.class_name,
                        studentCount: 0,
                        students: []
                    };
                }
                classSummary[record.class_name].studentCount++;
                classSummary[record.class_name].students.push({
                    studentId: record.student_id,
                    studentName: record.student_name,
                    attendanceTime: record.formatted_time,
                    confidence: record.confidence
                });
            });

            res.json({
                success: true,
                message: 'Today\'s attendance summary retrieved successfully',
                data: {
                    date: today,
                    stats: {
                        totalRecords: parseInt(stats.total_records) || 0,
                        uniqueStudents: parseInt(stats.unique_students) || 0,
                        uniqueClasses: parseInt(stats.unique_classes) || 0,
                        averageConfidence: parseFloat(stats.avg_confidence) || 0
                    },
                    classSummary: Object.values(classSummary),
                    recentAttendance: attendanceRecords.slice(0, 10)
                }
            });

        } catch (error) {
            console.error('Error in getTodayAttendanceSummary:', error);
            res.status(500).json({
                success: false,
                message: 'Internal server error',
                error: process.env.NODE_ENV === 'development' ? error.message : undefined
            });
        }
    }
    static euclideanDistance(d1, d2) {
        if (d1.length !== d2.length) return Infinity;
        return Math.sqrt(d1.reduce((sum, val, i) => sum + Math.pow(val - d2[i], 2), 0));
    }

static async recognizeFace(req, res) {
    try {
        const { descriptor } = req.body;

        if (!Array.isArray(descriptor) || descriptor.length !== 128) {
            return res.status(400).json({
                success: false,
                message: 'Descriptor không hợp lệ'
            });
        }

        // Lấy toàn bộ descriptor từ DB
        const allFaces = await FaceRecognitionModel.getAllFaceDescriptors();

        let bestMatch = null;
        let minDistance = Infinity;

        // So sánh descriptor mới với tất cả trong DB
        for (const face of allFaces) {
            const dbDescriptor = JSON.parse(face.descriptor);
            const distance = FaceRecognitionController.euclideanDistance(descriptor, dbDescriptor);

            if (distance < minDistance) {
                minDistance = distance;
                bestMatch = face;
            }
        }

        const THRESHOLD = 0.6;

        if (minDistance < THRESHOLD) {
            return res.json({
                success: true,
                message: 'Đã nhận diện thành công',
                name: bestMatch.name,
                studentId: bestMatch.id,
                className: bestMatch.class,
                distance: minDistance
            });
        } else {
            return res.json({
                success: false,
                message: 'Không khớp với sinh viên nào',
                distance: minDistance
            });
        }

    } catch (error) {
        console.error('Lỗi trong recognizeFace:', error);
        res.status(500).json({
            success: false,
            message: 'Lỗi server khi nhận diện',
            error: process.env.NODE_ENV === 'development' ? error.message : undefined
        });
    }
}

static async registerSignalFromESP32(req, res) {
    try {
        const { mssv } = req.body;

        if (!mssv) {
            return res.status(400).json({ success: false, message: 'Thiếu MSSV' });
        }

        // Gửi MSSV cho client web thông qua WebSocket / lưu lại / xử lý ngay
        // Ở đây ta có thể gọi 1 callback hoặc lưu `mssv` vào biến global và chờ `descriptor`

        console.log("📥 ESP32 yêu cầu đăng ký MSSV:", mssv);

        // TODO: Gửi tín hiệu sang faceRecognition.js (client) để lấy descriptor và gọi /register

        res.status(200).json({ success: true, message: 'Đã nhận MSSV từ ESP32' });
    } catch (error) {
        console.error("Lỗi trong registerSignalFromESP32:", error);
        res.status(500).json({ success: false, message: 'Lỗi server' });
    }
}
static async getFingerprintTemplate(req, res) {
    try {
        const id = req.query.id;
        if (!id) {
            return res.status(400).json({ error: 'Missing id' });
        }

        const [rows] = await require('../config/db').query(
            'SELECT template_base64 FROM fingerprint_templates WHERE id = ?', [id]
        );

        if (rows.length === 0) {
            return res.status(404).json({ error: 'Template not found' });
        }

        res.json({ template: rows[0].template_base64 });
    } catch (error) {
        console.error("❌ Lỗi khi lấy template:", error);
        res.status(500).json({ error: 'Server error' });
    }
}


}

module.exports = FaceRecognitionController;