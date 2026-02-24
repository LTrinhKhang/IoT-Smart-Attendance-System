// const API_BASE = `http://${window.location.hostname}:5500/api/workDuration`; // Cổng backend



async function calculateWorkDuration() {
    try {
        const response = await fetch(API_BASE,{                                 // Gửi yêu cầu lấy dữ liệu từ API
            method : 'GET',
            headers : {
                'content-type' : 'application/json'
            }
        }); 
        if (!response.ok) {                                                     //Kiểm tra kết quả trả về từ API
            const errorMessage = await response.json();
            throw new Error(errorMessage.message || "Lỗi khi lấy dữ liệu");
        }
        const workDurations = await response.json();                            //Chuyển đổi dữ liệu API về dạng JSON
        if (!Array.isArray(workDurations)) {
            throw new Error("Dữ liệu không hợp lệ");
        }
        const workDurationList = document.getElementById("workDurationList");
        workDurationList.innerHTML = workDurations.map(workDuration => `
            <tr>
                <td> ${workDuration.employeeId} </td>
                <td> ${workDuration.timestamp} giờ </td>
                <td> <button onclick="calculateWorkDuration(${workDuration.timestamp})">🗑 Tính Toán</button> </td>
            </tr>
`).join('');
        } catch (error) {
            console.error('Error calculating work duration:', error);
            alert('Không thể tính toán thời gian làm việc. Vui lòng kiểm tra kết nối.');
        }
}


// async function calculateWorkDuration() {
//     const selectedDate = document.getElementById("datePicker").value;
//     if (!selectedDate) {
//         alert("⚠️ Vui lòng chọn ngày!");
//         return;
//     }

//     try {
//         const localIP = window.location.hostname;
//         const response = await fetch(`http://${localIP}:5500/api/attendance`);
//         if (!response.ok) throw new Error('Server error');

//         const logs = await response.json();
//         const filteredLogs = logs.filter(log => 
//             new Date(log.timestamp).toISOString().split('T')[0] === selectedDate
//         );

//         const workDurations = {};
//         filteredLogs.forEach(log => {
//             const employeeId = log.employeeId;
//             const time = new Date(log.timestamp).getTime();

//             if (!workDurations[employeeId]) {
//                 workDurations[employeeId] = { in: null, out: null, total: 0 };
//             }

//             if (!workDurations[employeeId].in || time < workDurations[employeeId].in) {
//                 workDurations[employeeId].in = time; 
//             }

//             if (!workDurations[employeeId].out || time > workDurations[employeeId].out) {
//                 workDurations[employeeId].out = time; 
//             }
//         });

//         const workDurationList = document.getElementById("workDurationList");
//         workDurationList.innerHTML = ''; 

//         Object.entries(workDurations).forEach(([employeeId, times]) => {
//             if (times.in && times.out) {
//                 const duration = (times.out - times.in) / (1000 * 60 * 60);
//                 const row = document.createElement('tr');
//                 row.innerHTML = `<td>${employeeId}</td><td>${duration.toFixed(2)} giờ</td>`;
//                 workDurationList.appendChild(row);
//             }
//         });

//     } catch (error) {
//         console.error('Error calculating work duration:', error);
//         alert('Không thể tính toán thời gian làm việc. Vui lòng kiểm tra kết nối.');
//     }
// }   

//  Gán hàm vào `window` để có thể gọi từ HTML
window.calculateWorkDuration = calculateWorkDuration;
