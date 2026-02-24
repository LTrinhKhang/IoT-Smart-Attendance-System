const express = require("express");
const router = express.Router();
const authController = require("../controllers/authController");

// API Đăng ký
router.post("/register", authController.register);

// API Đăng nhập
router.post("/login", authController.login);

//  Export đúng cách
module.exports = router;
