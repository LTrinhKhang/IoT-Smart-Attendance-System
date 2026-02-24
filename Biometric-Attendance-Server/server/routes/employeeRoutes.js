const express = require('express');
const router = express.Router();
const { verifyToken, verifyAdmin } = require("../middleware/authMiddleware");
const employeeController = require('../controllers/employeeController'); // ✅ Import đúng

// API Lấy danh sách nhân viên (Yêu cầu token)
router.get("/", verifyToken, employeeController.getAllEmployees);

// API Lấy nhân viên theo ID
router.get('/:id', verifyToken, employeeController.getEmployeeById);

// API Thêm nhân viên (Chỉ Admin)
router.post("/", verifyToken, verifyAdmin, employeeController.createEmployee);

// API Xóa nhân viên (Chỉ Admin)
router.delete('/:id', verifyToken, verifyAdmin, employeeController.deleteEmployee);

module.exports = router;

/*

/api/employees/ → Lấy danh sách nhân viên.
/api/employees/:id → Lấy nhân viên theo ID.
/api/employees/ (POST) → Thêm nhân viên mới.
/api/employees/:id (DELETE) → Xóa nhân viên.

*/