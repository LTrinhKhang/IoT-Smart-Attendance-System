const db = require('../config/db'); // Import kết nối database

const Duration = {
    getDate : async () => {
        const [rows] = await db.query('SELECT * FROM attendance');
        return rows;
    },
}