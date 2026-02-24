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





// Hide the loading screen once the window is fully loaded
window.addEventListener("load", function() {
    document.getElementById("loading-screen").style.visibility = "hidden";
});

// Sample student data
const students = [
    { id: "SV001", name: "Nguyễn Văn A", class: "CNTT-K15", status: "✔️ Đã điểm danh", avatar: "images/avatar (1).jpg" },
    { id: "SV002", name: "Trần Thị B", class: "CNTT-K15", status: "❌ Vắng", avatar: "images/avatar (2).jpg" },
];

// Render the list of students
function renderStudents() {
    let container = document.querySelector(".student-cards");
    container.innerHTML = ""; // Clear existing content

    students.forEach(student => {
        let statusClass = student.status.includes("✔️") ? "success" : "failed";
        container.innerHTML += `
            <div class="student-card">
                <img src="${student.avatar}" alt="${student.name}">
                <h3>${student.name}</h3>
                <p>Mã SV: <strong>${student.id}</strong></p>
                <p>Lớp: <strong>${student.class}</strong></p>
                <p>Trạng thái: <span class="${statusClass}">${student.status}</span></p>
            </div>
        `;
    });
}


// Show notification messages
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

// // Export student data to Excel
// document.getElementById("exportExcel").addEventListener("click", function() {
//     let wb = XLSX.utils.book_new();
//     let wsData = [["Mã SV", "Tên", "Lớp", "Trạng thái"]];

//     document.querySelectorAll(".student-card").forEach(card => {
//         let id = card.querySelector("p:nth-child(3) strong").innerText;
//         let name = card.querySelector("h3").innerText;
//         let className = card.querySelector("p:nth-child(4) strong").innerText;
//         let status = card.querySelector("span").innerText;
//         wsData.push([id, name, className, status]);
//     });

//     let ws = XLSX.utils.aoa_to_sheet(wsData);
//     XLSX.utils.book_append_sheet(wb, ws, "Danh sách SV");
//     XLSX.writeFile(wb, "danh_sach_sinh_vien.xlsx");

//     showNotification("Xuất file Excel thành công!");
// });

document.addEventListener('DOMContentLoaded', function () {
    let calendarEl = document.getElementById('calendar');

    let calendar = new FullCalendar.Calendar(calendarEl, {
        initialView: 'dayGridMonth',
        locale: 'vi',
        firstDay: 1,
        headerToolbar: {
            left: 'prev,next today',
            center: 'title',
            right: 'dayGridMonth,resourceTimelineWeek'
        },
        views: {
            resourceTimelineWeek: {
                type: 'resourceTimeline',
                duration: { weeks: 1 },
                slotDuration: "01:00",
                slotMinTime: "06:00:00",
                slotMaxTime: "22:00:00",
            }
        },
        events: async function (fetchInfo, successCallback, failureCallback) {
            try {
                let response = await fetch('/api/class-schedule');
                let data = await response.json();
                successCallback(data);
            } catch (error) {
                console.error('Lỗi tải dữ liệu lịch:', error);
                failureCallback(error);
            }
        },
        eventClick: function (info) {
            alert(`${info.event.title}\nThời gian: ${info.event.startTime} - ${info.event.endTime}`);
        }
    });

    calendar.render();
});




// // Call the function to render students
// renderStudents();