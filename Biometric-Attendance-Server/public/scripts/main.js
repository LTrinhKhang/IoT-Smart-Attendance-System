// Wait for the DOM to be fully loaded
document.addEventListener("DOMContentLoaded", function() {
    const sidebar = document.querySelector(".sidebar");
    const content = document.querySelector(".main-content");

    // Adjust main content margin when sidebar is hovered
    sidebar.addEventListener("mouseenter", function() {
        content.style.marginLeft = "230px";
    });

    sidebar.addEventListener("mouseleave", function() {
        content.style.marginLeft = "80px";
    });
});

// Load thư viện Chart.js
const ctx = document.getElementById('attendanceChart').getContext('2d');

// Dữ liệu ban đầu
let dataValues = [120, 110, 130, 90, 150];

// Hàm lấy giá trị cao nhất để vẽ biểu đồ hợp lý
function getMaxValue(data) {
    const max = Math.max(...data);
    return Math.ceil(max * 1.2); // Giữ khoảng trống trên cùng
}

// Tạo hàm để khởi tạo biểu đồ
let attendanceChart;
function createChart() {
    if (attendanceChart) {
        attendanceChart.destroy(); // Hủy biểu đồ cũ trước khi tạo lại
    }
    attendanceChart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: ['Thứ 2', 'Thứ 3', 'Thứ 4', 'Thứ 5', 'Thứ 6'],
            datasets: [{
                label: 'Số sinh viên điểm danh',
                data: dataValues,
                backgroundColor: ['#4CAF50', '#FF9800', '#2196F3', '#9C27B0', '#FF5722'],
                borderWidth: 5,
                barPercentage: 0.9, 
                categoryPercentage: 0.9
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false, // Tự điều chỉnh theo container
            scales: {
                x: {
                    grid: {
                        display: false
                    }
                },
                y: {
                    beginAtZero: true,
                    suggestedMax: getMaxValue(dataValues)
                }
            },
            plugins: {
                legend: {
                    display: false
                }
            }
        }
    });
}


function showNotification(message) {
    let container = document.getElementById("notification-container");
    let notification = document.createElement("div");
    notification.classList.add("notification", "show");
    notification.innerText = message;

    container.appendChild(notification);

    setTimeout(() => {
        notification.classList.remove("show");
        notification.classList.add("hide");
        setTimeout(() => notification.remove(), 500);
    }, 3000);
}

// Export student data to Excel
document.getElementById("exportExcel").addEventListener("click", function() {
    let wb = XLSX.utils.book_new();
    let wsData = [["Mã SV", "Tên", "Lớp", "Trạng thái"]];

    document.querySelectorAll(".student-card").forEach(card => {
        let id = card.querySelector("p:nth-child(3) strong").innerText;
        let name = card.querySelector("h3").innerText;
        let className = card.querySelector("p:nth-child(4) strong").innerText;
        let status = card.querySelector("span").innerText;
        wsData.push([id, name, className, status]);
    });

    let ws = XLSX.utils.aoa_to_sheet(wsData);
    XLSX.utils.book_append_sheet(wb, ws, "Danh sách SV");
    XLSX.writeFile(wb, "danh_sach_sinh_vien.xlsx");

    showNotification("Xuất file Excel thành công!");
});

// Call the function to render students
renderStudents();