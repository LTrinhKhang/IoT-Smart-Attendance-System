document.addEventListener("DOMContentLoaded", async function () {
  const classListContainer = document.getElementById("class-list");

  try {
      const response = await fetch(window.API_ROUTES.attendance);
      const data = await response.json();

      if (data.success && data.classes.length > 0) {
          classListContainer.innerHTML = ""; // Xóa nội dung cũ

          data.classes.forEach(classItem => {
              const classCard = document.createElement("div");
              classCard.classList.add("card");

              classCard.innerHTML = `
                  <i class="fi fi-rr-graduation-cap"></i>
                  <h3>${classItem.class_name}</h3>
                  <p><strong>Thời gian:</strong> ${classItem.start_time} - ${classItem.end_time}</p>
              `;

              classListContainer.appendChild(classCard);
          });
      } else {
          classListContainer.innerHTML = "<p>Không có lớp học nào hôm nay</p>";
      }
  } catch (error) {
      console.error("Lỗi khi tải danh sách lớp:", error);
      classListContainer.innerHTML = "<p>Lỗi tải dữ liệu</p>";
  }
});
