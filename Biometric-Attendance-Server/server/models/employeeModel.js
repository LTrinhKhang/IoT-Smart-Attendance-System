const db = require('../config/db'); // Import kết nối database

const Employee = {
    getAll: async () => {
        const [rows] = await db.query('SELECT * FROM employees');
        return rows;
    },

    getById: async (employeeId) => {
        const [rows] = await db.execute('SELECT * FROM employees WHERE employeeId = ?', [employeeId]);
        return rows[0];
    },

    create: async (name) => {
        const [result] = await db.execute('INSERT INTO employees (name) VALUES (?)', [name]);
        return result.insertId;
    },

    delete: async (employeeId) => {
        await db.execute('DELETE FROM employees WHERE employeeId = ?', [employeeId]);
    }
};

module.exports = Employee;
/*

getAll(): Lấy danh sách tất cả nhân viên.
getById(id): Lấy một nhân viên theo employeeId.
create(name): Thêm một nhân viên mới.
delete(id): Xoá nhân viên theo employeeId.

*/
