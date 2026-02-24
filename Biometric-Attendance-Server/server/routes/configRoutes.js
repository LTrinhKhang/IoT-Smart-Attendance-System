const express = require("express");
const router = express.Router();

router.get("/config", (req, res) => {
    res.json({
        auth: process.env.API_AUTH,
        attendance: process.env.API_ATTENDANCE,
        employees: process.env.API_EMPLOYEES,
        upload: process.env.API_UPLOAD
    });
});

module.exports = router;
