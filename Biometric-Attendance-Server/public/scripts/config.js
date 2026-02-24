window.API_ROUTES = {
    auth: `http://${window.location.hostname}:5500/api/auth`,
    attendance: `http://${window.location.hostname}:5500/api/attendance`,
    employees: `http://${window.location.hostname}:5500/api/employees`,
    upload: `http://${window.location.hostname}:5500/api/upload`,
    recognize: `http://${window.location.hostname}:5500/api/recognize`
};

// Cấu hình ESP32 Camera
window.ESP32_CONFIG = {
    defaultIpAddress: '192.168.0.3',
    streamPort: 81
    controlPort: 80
};

