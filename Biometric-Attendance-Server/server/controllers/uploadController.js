const UploadModel = require("../models/uploadModel");

exports.uploadFile = async (req, res) => {
    try {
        console.log("Nhận request upload!");
        const { className, students, classDay, classStartTime, classEndTime } = req.body;

        // Kiểm tra dữ liệu đầu vào
        if (!className || !students || students.length === 0) {
            return res.status(400).json({ success: false, message: "Dữ liệu không hợp lệ!" });
        }

        // Tạo bảng cho lớp học nếu chưa có
        await UploadModel.createTable(className);

        // Chèn danh sách sinh viên vào database
        await UploadModel.insertStudents(className, students);

        // Trả về kết quả thành công
        res.json({ 
            success: true,
            message: `Đã lưu lớp ${className} với ${students.length} sinh viên vào database!`
        });

    } catch (error) {
        console.error(" Lỗi khi xử lý dữ liệu:", error);
        res.status(500).json({ 
            success: false,
            message: "Lỗi server khi xử lý dữ liệu!"
        });
    }
};
