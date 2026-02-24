const attendanceModel = require ("../models/attendanceModel");


exports.getTodayClasses = async (req, res) => {
    try{
        console.log("Nhận request lấy danh sách lớp học hôm nay!");
        const todayClasses = await attendanceModel.getTodayClasses(); // Lấy danh sách lớp học hôm nay từ model
        console.log(" Danh sách lớp hôm nay:", todayClasses);
    }
    



    catch(error){
        console.error(" Lỗi API /classes/today:", error);
        res.status(500).json({ success: false, message: "Lỗi server", error: error.message });
    }
};