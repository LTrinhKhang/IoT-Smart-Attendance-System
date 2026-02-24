document.addEventListener("DOMContentLoaded", function() {
    const fileInput = document.getElementById("excelFile");
    const uploadBtn = document.getElementById("uploadFileBtn");
    const classInfoSection = document.getElementById("classInfoSection");
    const classInfo = document.getElementById("classInfo");
    const studentListSection = document.getElementById("studentListSection");
    const previewTable = document.getElementById("previewTable");
    const saveBtn = document.getElementById("saveToDB");
    const cancelBtn = document.getElementById("cancelUpload");
    if (!window.API_ROUTES) {
        alert("Chưa tải xong API, vui lòng thử lại!");
        return;
    }

    saveBtn.addEventListener("click", async function () {
        const className = document.getElementById("classNameInput").value.trim();
        const classDay = document.getElementById("classDay").value;
        const classStartTime = document.getElementById("classStartTime").value;
        const classEndTime = document.getElementById("classEndTime").value;
    
        if (!className || !classDay || !classStartTime || !classEndTime) {
            alert("Vui lòng nhập đầy đủ thông tin lớp học!");
            return;
        }
    
        // Get student data from the preview table
        const students = [];
        const rows = previewTable.querySelectorAll("tbody tr");
        rows.forEach(row => {
            const cells = row.querySelectorAll("td");
            students.push({
                student_id: cells[1].textContent,
                name: cells[2].textContent,
                class_day: classDay,
                start_time: classStartTime,
                end_time: classEndTime
            });
        });
    
        try {
            const response = await fetch(window.API_ROUTES.upload, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ 
                    className, 
                    students,
                    classDay,
                    classStartTime, 
                    classEndTime
                })
            });
    
            // First check if the request succeeded
            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`Server error: ${response.status} - ${errorText}`);
            }
    
            // Then try to parse as JSON
            const data = await response.json();
            alert(data.message);
        } catch (error) {
            console.error("Full error:", error);
            alert(`Lỗi kết nối: ${error.message}`);
        }
    });

    cancelBtn.addEventListener("click", function () {
        document.getElementById("additionalInfo").classList.add("hidden");
    });

    uploadBtn.addEventListener("click", function() {
        const file = fileInput.files[0];
        if (!file) {
            alert("Vui lòng chọn file Excel!");
            return;
        }

        const reader = new FileReader();
        reader.onload = function(e) {
            const data = new Uint8Array(e.target.result);
            const workbook = XLSX.read(data, { type: 'array' });
            const firstSheet = workbook.Sheets[workbook.SheetNames[0]];
            const jsonData = XLSX.utils.sheet_to_json(firstSheet, { header: 1 });

            // Hiển thị các section
            classInfoSection.classList.remove("hidden");
            studentListSection.classList.remove("hidden");

            // Trích xuất thông tin lớp học
            const classInfoData = extractClassInfo(jsonData);
            displayClassInfo(classInfoData);

            // Trích xuất danh sách sinh viên
            const studentData = extractStudentData(jsonData);
            displayStudentData(studentData.headers, studentData.rows);
        };
        reader.readAsArrayBuffer(file);
    });

    function extractClassInfo(data) {
        const info = {
            semester: "N/A",
            academicYear: "N/A",
            course: "N/A",
            courseCode: "N/A",
            group: "N/A",
            teacher: "N/A",
            teacherCode: "N/A",
            credits: "N/A",
            className: "N/A",
            suggestions: []
        };

        for (let i = 0; i < Math.min(data.length, 10); i++) {
            if (!data[i] || !data[i][0]) continue;
            
            const cellValue = data[i][0]?.toString().trim() || '';
            
            // Xử lý học kỳ
            if (cellValue.includes("Học kỳ:")) {
                const parts = cellValue.split("Học kỳ:")[1]?.split("- Năm học:") || [];
                info.semester = parts[0]?.replace(/-/g, "").trim() || "N/A";
                if (parts[1]) info.academicYear = parts[1].trim();
                continue;
            }
            // Xử lý môn học và nhóm
            if (cellValue.includes("Môn học/Nhóm:")) {
                const courseParts = cellValue.split("Môn học/Nhóm:")[1]?.split("- Nhóm") || [];
                
                if (courseParts[0]) {
                    const courseInfo = courseParts[0].trim();
                    info.course = courseInfo.split("(")[0].trim();
                    const codeMatch = courseInfo.match(/\(([^)]+)\)/);
                    if (codeMatch) {
                        info.courseCode = codeMatch[1];
                    }
                }
                
                if (courseParts[1]) {
                    info.group = courseParts[1].trim();
                    info.className = `${info.courseCode}-${info.group}`;
                }
                continue;
            }
            
            // Xử lý giảng viên
            if (cellValue.includes("CBGD:")) {
                const teacherPart = cellValue.split("CBGD:")[1]?.trim() || "";
                info.teacher = teacherPart.split("(")[0].trim();
                const codeMatch = teacherPart.match(/\(([^)]+)\)/);
                if (codeMatch) info.teacherCode = codeMatch[1];
                
                // Xử lý số tín chỉ ở cột R (index 17)
                if (data[i][17]?.toString().includes("Số tín chỉ:")) {
                    info.credits = data[i][17].toString().split("Số tín chỉ:")[1]?.trim() || "N/A";
                }
            }
        }

        return info;
    }

    function displayClassInfo(info) {
        const suggestionsHTML = info.suggestions.length > 0 ? `
        <div class="suggestions-box">
            <h4>Gợi ý tên lớp:</h4>
            <div class="suggestions">
                ${info.suggestions.map(s => `
                <div class="suggestion-badge" onclick="document.getElementById('classNameInput').value='${s}'">
                    ${s}
                </div>
                `).join('')}
            </div>
        </div>
        ` : '';

        const html = `
        <div class="class-info-header">
            <h3>Thông tin lớp học</h3>
            <div class="class-name-input">
                <label>Tên lớp:</label>
                <input type="text" id="classNameInput" value="${info.className}" 
                       placeholder="Nhập tên lớp theo định dạng Mã môn+Nhóm">
            </div>
        </div>
        
        <div class="class-details-grid">
            <div class="detail-item">
                <span class="detail-label">Học kỳ:</span>
                <span class="detail-value">${info.semester}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Năm học:</span>
                <span class="detail-value">${info.academicYear}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Môn học:</span>
                <span class="detail-value">${info.course}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Mã môn:</span>
                <span class="detail-value">${info.courseCode}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Nhóm:</span>
                <span class="detail-value">${info.group}</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Giảng viên:</span>
                <span class="detail-value">${info.teacher} (${info.teacherCode})</span>
            </div>
            <div class="detail-item">
                <span class="detail-label">Số tín chỉ:</span>
                <span class="detail-value">${info.credits}</span>
            </div>
        </div>
        
        ${suggestionsHTML}
        `;
        
        classInfo.innerHTML = html;
    }

    function extractStudentData(data) {
        const result = { headers: [], rows: [] };
        let headerRowIndex = -1;

        // Tìm dòng tiêu đề
        for (let i = 0; i < Math.min(data.length, 15); i++) {
            if (data[i] && data[i][0] === "TT" && data[i][1] === "Mã SV") {
                headerRowIndex = i;
                break;
            }
        }

        if (headerRowIndex >= 0) {
            // Lấy tiêu đề (bỏ qua các cột trống không cần thiết)
            result.headers = data[headerRowIndex].filter((cell, index) => 
                cell || index <= 5 // Giữ lại các cột chính dù có thể trống
            );
            
            // Lấy dữ liệu sinh viên
            for (let i = headerRowIndex + 1; i < data.length; i++) {
                if (data[i] && data[i][0] && !isNaN(data[i][0])) {
                    const rowData = data[i].filter((_, index) => 
                        index < result.headers.length
                    );
                    result.rows.push(rowData);
                }
            }
        }

        return result;
    }

    function displayStudentData(headers, rows) {
        const thead = previewTable.querySelector("thead");
        const tbody = previewTable.querySelector("tbody");
        
        thead.innerHTML = "";
        tbody.innerHTML = "";

        if (headers.length > 0) {
            let headerRow = "<tr>";
            headers.forEach((header, index) => {
                const colName = index === 0 ? "STT" : 
                              index === 1 ? "Mã SV" : 
                              index === 2 ? "Họ và tên" : 
                              index === 3 ? "Ngày sinh" : 
                              index === 4 ? "Lớp" :  
                              header || `Buổi ${index - 5}`;
                headerRow += `<th>${colName}</th>`;
            });
            headerRow += "</tr>";
            thead.innerHTML = headerRow;
        }

        rows.forEach(row => {
            let rowHTML = "<tr>";
            row.forEach((cell, index) => {
                if (index === 2 && row[3]) { // Ghép họ và tên
                    rowHTML += `<td>${cell || ""} ${row[3] || ""}</td>`;
                } 
                else if (index === 3) { // Bỏ qua cột tên riêng đã ghép
                    return;
                }
                else {
                    rowHTML += `<td>${cell || ""}</td>`;
                }
            });
            rowHTML += "</tr>";
            tbody.innerHTML += rowHTML;
        });
    }
});



