const express = require('express');
const router = express.Router();
const registrationController = require('../controllers/registrationController');

// Kiểm tra trạng thái sinh viên (đã đăng ký khuôn mặt và vân tay chưa)
router.get('/student-status', registrationController.checkStudentStatus);

// Đăng ký khuôn mặt
router.post('/register-face', registrationController.registerFace);

// Đăng ký vân tay
router.post('/register-template', registrationController.registerTemplate);

module.exports = router;