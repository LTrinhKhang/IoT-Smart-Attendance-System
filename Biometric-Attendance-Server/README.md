# Attendance System Server

This is the backend server for the IoT Smart Attendance System. It provides a REST API for the ESP32-S3 device and a web dashboard for management.

## Features

- **Employee/Student Management**: CRUD operations for attendance participants.
- **Attendance Logging**: Real-time logging from both fingerprint and face recognition.
- **Face Recognition**: Integrated face recognition using `face-api.js` (client-side) and server-side storage.
- **Real-time Dashboard**: Monitor attendance as it happens.
- **Data Export**: Export attendance records (e.g., to Excel/XLSX).

## Tech Stack

- **Backend**: Node.js, Express.
- **Database**: MySQL.
- **Frontend**: HTML, CSS, JavaScript (Vanilla), FullCalendar.

## Setup Instructions

### 1. Prerequisites
- Node.js installed.
- MySQL Server running.

### 2. Database Configuration
Create a `.env` file in the `server/` root with the following variables:
```env
DB_HOST=localhost
DB_USER=root
DB_PASSWORD=your_password
DB_NAME=attendance_db
PORT=5500
```

*Note: Ensure you have a `config/db.js` that connects to MySQL using these credentials.*

### 3. Installation
1.  Navigate to the server directory:
    ```bash
    cd server
    ```
2.  Install dependencies:
    ```bash
    npm install
    ```

### 4. Running the Server
```bash
node server/index.js
```
The server will start on `http://localhost:5500`.

## File Structure

- `server/index.js`: Main entry point.
- `server/controllers/`: Business logic for each route.
- `server/models/`: Database schemas and queries.
- `server/routes/`: API endpoint definitions.
- `public/`: Frontend assets (HTML, CSS, JS).
- `public/faces/`: Stored face images for recognition.
