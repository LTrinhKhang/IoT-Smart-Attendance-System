const express = require('express');
const router = express.Router();
const faceRecognitionController = require('../controllers/faceRecognitionController');
const faceSignalController = require('../controllers/faceSignalController');

// Route nhận diện
router.post('/recognize', faceRecognitionController.recognizeFace);

// Route ghi điểm danh
router.post('/attendance', faceRecognitionController.recordAttendance);

// Route lấy danh sách face descriptors
router.get('/descriptors', faceRecognitionController.getAllFaceDescriptors);

// Route đăng ký face descriptor
router.post('/register', faceRecognitionController.registerFace);

// Route lấy thống kê điểm danh
router.get('/stats', faceRecognitionController.getAttendanceStats);

// Route lấy bản ghi điểm danh
router.get('/records', faceRecognitionController.getAttendanceRecords);

// Route lấy tóm tắt điểm danh hôm nay
router.get('/today', faceRecognitionController.getTodayAttendanceSummary);

// ✅ Tín hiệu đăng ký sinh viên từ ESP32 → Web
router.post('/signal/face-register', faceSignalController.receiveRegisterSignal);
router.get('/signal/pending-register', faceSignalController.getPendingRegisterSignal);

// ✅ Tín hiệu điểm danh từ ESP32 → Web
router.post('/signal/face-attendance', faceSignalController.receiveAttendanceSignal);

// ✅ Frontend polling để kiểm tra tín hiệu điểm danh
router.get('/signal/attendance', faceSignalController.getAttendanceSignal);

router.get('/fingerprint-template', faceRecognitionController.getFingerprintTemplate);

// Health check
router.get('/health', (req, res) => {
    res.json({
        success: true,
        message: 'Face Recognition API is running',
        timestamp: new Date().toISOString()
    });
});

module.exports = router;
