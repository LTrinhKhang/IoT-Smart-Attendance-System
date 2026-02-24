const Employee = require('../models/employeeModel');

const getAllEmployees = async (req, res) => {
    try {
        const employees = await Employee.getAll();

        console.log(" Employees fetched:", employees); // Debug

        if (!Array.isArray(employees)) {
            return res.status(500).json({ message: "Dữ liệu trả về không hợp lệ" });
        }

        res.json(employees);
    } catch (error) {
        console.error(" Lỗi khi lấy danh sách nhân viên:", error);
        res.status(500).json({ message: "Lỗi server", error });
    }
};


const getEmployeeById = async (req, res) => {
    try {
        const employee = await Employee.getById(req.params.id);
        if (!employee) return res.status(404).json({ message: 'Nhân viên không tồn tại' });

        res.json(employee);
    } catch (error) {
        res.status(500).json({ message: 'Lỗi khi lấy thông tin nhân viên', error });
    }
};

const createEmployee = async (req, res) => {
    try {
        const { name } = req.body;
        if (!name) return res.status(400).json({ message: 'Tên nhân viên là bắt buộc' });

        const employeeId = await Employee.create(name);
        res.status(201).json({ message: 'Thêm nhân viên thành công', employeeId });
    } catch (error) {
        res.status(500).json({ message: 'Lỗi khi thêm nhân viên', error });
    }
};

const deleteEmployee = async (req, res) => {
    try {
        await Employee.delete(req.params.id);
        res.json({ message: 'Xóa nhân viên thành công' });
    } catch (error) {
        res.status(500).json({ message: 'Lỗi khi xóa nhân viên', error });
    }
};

module.exports = { getAllEmployees, getEmployeeById, createEmployee, deleteEmployee };

/*

getAllEmployees(): Lấy toàn bộ danh sách nhân viên.
getEmployeeById(): Lấy thông tin một nhân viên.
createEmployee(): Tạo nhân viên mới.
deleteEmployee(): Xóa nhân viên. 

*/