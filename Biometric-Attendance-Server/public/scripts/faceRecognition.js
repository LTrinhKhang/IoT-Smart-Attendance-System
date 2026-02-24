class FaceRecognitionSystem {
    constructor() {
        this.esp32Stream = document.getElementById('esp32Stream');
        this.overlay = document.getElementById('overlay');
        this.canvas = document.createElement('canvas');
        this.context = this.canvas.getContext('2d');
        this.isVideoPlaying = false;
        this.currentMode = 'attendance'; // hoặc 'register', 'idle', etc.
        
        // Lấy IP địa chỉ từ localStorage nếu có, nếu không thì dùng cấu hình mặc định
        this.esp32IpAddress = localStorage.getItem('esp32IpAddress') || 
                              (window.ESP32_CONFIG?.defaultIpAddress || '192.168.0.3');
        this.esp32StreamPort = window.ESP32_CONFIG?.streamPort || 81;

        // Luồng điểm danh tự động
        this.attendanceInterval = null;
        this.isAttendanceMode = false;
        
        // Luồng đăng ký tự động
        this.registerPollingInterval = null;
        this.isRegisterMode = false;
        this.pendingRegisterMSSV = null;
        this.isProcessingRegistration = false;
        
        // Luồng polling tín hiệu từ ESP32
        this.signalPollingInterval = null;
        
        // Stream connection status
        this.isStreamConnected = false;
        this.streamCheckInterval = null;
        this.streamReconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
        
        // Tracking các thông báo và lỗi
        this.lastNotification = null;
        this.notificationQueue = [];
        this.lastErrorNotification = 0;
        this.lastConnectionErrorNotification = 0;
        this.lastActivityKey = null;
        this.lastActivityTime = 0;
        this.lastPollingError = 0;
        
        // Tạo notification container nếu chưa có
        this.ensureNotificationContainer();
        
        // Ẩn loading screen khi trang đã tải xong
        this.hideLoadingScreen();
        
        console.log(`ESP32 Camera sẽ kết nối đến: ${this.esp32IpAddress}:${this.esp32StreamPort}`);
        
        // Tự động khởi tạo stream và polling
        this.initializeAutoSystem();
    }
    updateSignalStatus(type, status) {
    const elId = type === 'attendance' ? 'attendanceStatus' : 'registerStatus';
    const statusEl = document.getElementById(elId);
    if (statusEl) {
        statusEl.textContent = status ? 'Đang hoạt động' : 'Đang chờ';
        statusEl.className = status ? 'status-badge active' : 'status-badge';
    }
    }

    updateModeDisplay(mode, message) {
        const statusEl = document.getElementById("modeStatus");
        if (statusEl) {
            statusEl.textContent = message || `Chế độ: ${mode}`;
        }
    }
    // Bắt đầu luồng điểm danh tự động
    startAttendanceMode() {
        if (this.attendanceInterval) return;
        
        this.isAttendanceMode = true;
        this.isRegisterMode = false;
        this.updateModeDisplay('attendance', 'Chế độ điểm danh đang hoạt động');
        this.updateSignalStatus('attendance', true);
        
        console.log('🎯 Bắt đầu chế độ điểm danh tự động...');
        this.addActivity("Chế độ điểm danh", "Hệ thống chuyển sang chế độ điểm danh tự động", "info");

        this.attendanceInterval = setInterval(() => {
            this.processAttendanceCapture();
        }, 3000); // Mỗi 3 giây kiểm tra điểm danh
    }

    // Bắt đầu luồng đăng ký tự động
    startRegisterMode(mssv) {
        this.pendingRegisterMSSV = mssv;  // ✅ Gán lại MSSV sau khi stopAllModes()

        if (this.registerPollingInterval) return;

        this.isRegisterMode = true;
        this.isAttendanceMode = false;
        this.updateModeDisplay('register', 'Chế độ đăng ký đang hoạt động');
        this.updateSignalStatus('register', true);

        console.log('📝 Bắt đầu chế độ đăng ký tự động...');
        this.addActivity("Chế độ đăng ký", `Hệ thống chuyển sang chế độ đăng ký tự động cho MSSV ${mssv}`, "info");

        this.registerPollingInterval = setInterval(() => {
            this.processRegistrationCapture();
        }, 2000); // Mỗi 2 giây kiểm tra đăng ký
    }


    // Dừng tất cả các chế độ
    stopAllModes() {
        // Dừng điểm danh
        if (this.attendanceInterval) {
            clearInterval(this.attendanceInterval);
            this.attendanceInterval = null;
        }
        
        // Dừng đăng ký
        if (this.registerPollingInterval) {
            clearInterval(this.registerPollingInterval);
            this.registerPollingInterval = null;
        }
        
        this.isAttendanceMode = false;
        this.isRegisterMode = false;
        this.pendingRegisterMSSV = null;
        this.isProcessingRegistration = false;
        
        this.updateModeDisplay('idle', 'Đang chờ tín hiệu từ ESP32...');
        this.updateSignalStatus('attendance', false);
        this.updateSignalStatus('register', false);
        
        console.log('⏸️ Đã dừng tất cả các chế độ tự động');
        }

    // Xử lý capture cho điểm danh
    async processAttendanceCapture() {
        if (!this.isAttendanceMode) return;
        
        // Đảm bảo stream kết nối trước khi xử lý
        const streamReady = await this.ensureStreamConnection();
        if (!streamReady) {
            console.error('❌ Không thể kết nối stream để điểm danh');
            return;
        }
        
        try {
            const img = document.getElementById('esp32Snapshot');
            if (!img) {
                console.error('Không tìm thấy phần tử esp32Snapshot');
                return;
            }

            img.crossOrigin = "anonymous";
            const captureUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/capture?ts=${Date.now()}`;
            
            img.src = captureUrl;

            img.onload = async () => {
                try {
                    if (img.naturalWidth === 0 || img.naturalHeight === 0) {
                        throw new Error('Ảnh không hợp lệ');
                    }

                    this.canvas.width = img.naturalWidth;
                    this.canvas.height = img.naturalHeight;
                    this.context.drawImage(img, 0, 0, img.naturalWidth, img.naturalHeight);

                    await new Promise(resolve => setTimeout(resolve, 100));

                    const descriptor = await this.extractFaceDescriptor(img);
                    await this.processAttendanceRecognition(descriptor);
                    
                } catch (err) {
                    console.warn("Không nhận diện được trong chế độ điểm danh:", err.message);
                }
            };

            img.onerror = (error) => {
                console.error('Lỗi load ảnh trong chế độ điểm danh:', error);
            };

        } catch (error) {
            console.error('Lỗi xử lý capture điểm danh:', error);
        }
    }

    // Xử lý capture cho đăng ký
    async processRegistrationCapture() {
    if (!this.isRegisterMode) return;

    // Đảm bảo stream kết nối trước khi xử lý
    const streamReady = await this.ensureStreamConnection();
    if (!streamReady) {
        console.error('❌ Không thể kết nối stream để đăng ký');
        return;
    }

    try {
        const img = document.getElementById('esp32Snapshot');
        if (!img) {
            console.error('❌ Không tìm thấy phần tử esp32Snapshot');
            return;
        }

        img.crossOrigin = "anonymous";
        const captureUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/capture?ts=${Date.now()}`;
        img.src = captureUrl;

        img.onload = async () => {
            try {
                if (img.naturalWidth === 0 || img.naturalHeight === 0) {
                    throw new Error('❌ Ảnh không hợp lệ hoặc không tải được');
                }

                // Vẽ ảnh vào canvas
                this.canvas.width = img.naturalWidth;
                this.canvas.height = img.naturalHeight;
                this.context.drawImage(img, 0, 0, img.naturalWidth, img.naturalHeight);

                // Đợi 1 chút để canvas render xong
                await new Promise(resolve => setTimeout(resolve, 100));

                // Trích xuất descriptor từ ảnh
                const descriptor = await this.extractFaceDescriptor(img);

                if (!descriptor) {
                    console.warn("⚠️ Không trích xuất được descriptor");
                    return;
                }

                // Gán descriptor vừa nhận được
                this.lastDescriptor = descriptor;

                console.log("✅ Đã trích xuất descriptor từ ảnh");
                console.log("🔎 pendingRegisterMSSV:", this.pendingRegisterMSSV);
                console.log("🔁 isProcessingRegistration:", this.isProcessingRegistration);

                // Nếu có MSSV đang chờ đăng ký, và chưa xử lý
                if (this.pendingRegisterMSSV && !this.isProcessingRegistration) {
                    console.log("➡️ Gọi đăng ký descriptor cho MSSV:", this.pendingRegisterMSSV);
                    await this.processRegisterDescriptor(this.pendingRegisterMSSV);
                } else {
                    if (!this.pendingRegisterMSSV)
                        console.warn("⚠️ Chưa có MSSV nào đang chờ đăng ký (pendingRegisterMSSV == null)");
                    if (this.isProcessingRegistration)
                        console.warn("⚠️ Hệ thống đang trong quá trình đăng ký khác (isProcessingRegistration == true)");
                }

            } catch (err) {
                console.warn("❌ Không nhận diện được trong chế độ đăng ký:", err.message);
            }
        };

        img.onerror = (error) => {
            console.error('❌ Lỗi tải ảnh từ ESP32:', error);
        };

    } catch (error) {
        console.error('❌ Lỗi chung trong processRegistrationCapture():', error);
    }
}


    // Xử lý nhận diện để điểm danh
    async processAttendanceRecognition(descriptor) {
        try {
            const response = await fetch('/api/face-recognition/recognize', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ descriptor: Array.from(descriptor) })
            });

            const result = await response.json();
            
            if (result.success) {
                this.showNotification(`✅ Điểm danh: ${result.name} (${result.className})`, 'success');
                this.addActivity("Điểm danh thành công", `${result.name} - ${result.className}`, "success");
                
                // Ghi nhận điểm danh
                await this.recordAttendance(result);
                
                // Gửi tín hiệu thành công cho ESP32
                await this.sendSuccessSignalToESP32('attendance', result);
                
                // Dừng chế độ điểm danh và quay lại polling sau 3 giây
                this.stopAllModes();
                setTimeout(() => {
                    console.log('🔄 Khởi động lại signal polling sau điểm danh thành công');
                    this.startSignalPolling();
                }, 3000);
                
            } else {
                console.log('Không nhận diện được người dùng trong chế độ điểm danh');
            }
        } catch (error) {
            console.error('Lỗi xử lý nhận diện điểm danh:', error);
        }
    }

    // Xử lý đăng ký descriptor với MSSV
    async processRegisterDescriptor(mssv) {
        if (this.isProcessingRegistration) return;
        
        this.isProcessingRegistration = true;
        // this.pendingRegisterMSSV = null;
        
        try {
            console.log(`📝 Đang đăng ký khuôn mặt cho MSSV: ${mssv}`);
            this.showNotification(`Đang đăng ký khuôn mặt cho MSSV: ${mssv}`, 'info');

            const response = await fetch('/api/face-recognition/register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    id: mssv,
                    descriptor: Array.from(this.lastDescriptor)
                })
            });

            const result = await response.json();
            if (result.success) {
                console.log(`✅ Đăng ký thành công cho MSSV: ${mssv}`);
                this.showNotification(`✅ Đăng ký khuôn mặt thành công cho MSSV ${mssv}`, 'success');
                this.addActivity("Đăng ký khuôn mặt", `Đăng ký thành công cho MSSV ${mssv}`, "success");

                // Gửi tín hiệu thành công cho ESP32
                await this.sendSuccessSignalToESP32('register', { mssv });
                
                // Tạm dừng chế độ đăng ký trong 5 giây
                this.stopAllModes();
                setTimeout(() => {
                    console.log('🔄 Khởi động lại signal polling sau đăng ký thành công');
                    this.startSignalPolling();
                }, 5000);
                
            } else {
                console.error(`❌ Đăng ký thất bại cho MSSV ${mssv}:`, result.message);
                this.showNotification(`❌ Đăng ký thất bại: ${result.message}`, 'error');
                this.addActivity("Đăng ký thất bại", `MSSV ${mssv}: ${result.message}`, "error");
            }
        } catch (error) {
            console.error('❌ Lỗi đăng ký descriptor:', error);
            this.showNotification('❌ Lỗi khi đăng ký khuôn mặt', 'error');
            this.addActivity("Lỗi đăng ký", `MSSV ${mssv}: ${error.message}`, "error");
        } finally {
            this.isProcessingRegistration = false;
        }
    }

    // Gửi tín hiệu thành công về ESP32
    async sendSuccessSignalToESP32(type, data) {
        try {
            let endpoint = '';
            let payload = {};
            
            if (type === 'attendance') {
                endpoint = '/attendance-success';
                payload = { 
                    name: data.name, 
                    mssv: data.studentId, // ✅ Sửa lại key này để trùng với ESP32
                    className: data.className || '' // optional
                };
            } else if (type === 'register') {
                endpoint = '/register-success';
                payload = { mssv: data.studentId };
            }
            
            await fetch(`http://${this.esp32IpAddress}:81${endpoint}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });

            console.log(`📤 Đã gửi tín hiệu ${type} thành công cho ESP32`);
        } catch (error) {
            console.warn(`⚠️ Không thể gửi tín hiệu ${type} cho ESP32:`, error);
        }
    }


    // Bắt đầu polling tín hiệu từ ESP32
    startSignalPolling() {
        if (this.signalPollingInterval) {
            console.log("⚠️ Signal polling đã đang chạy, bỏ qua việc khởi tạo mới");
            return;
        }
        
        console.log("🔄 Bắt đầu polling tín hiệu từ ESP32");
        
        this.signalPollingInterval = setInterval(async () => {
            try {
                // Kiểm tra tín hiệu điểm danh
                const attendanceRes = await fetch("/api/face-recognition/signal/attendance");
                const attendanceData = await attendanceRes.json();
                
                if (attendanceData.success && attendanceData.signal) {
                    console.log("📥 Nhận tín hiệu điểm danh từ ESP32");
                    this.stopSignalPolling(); // ✅ Dừng signal polling trước
                    this.stopAllModes();
                    this.startAttendanceMode();
                    return; // Chỉ xử lý một tín hiệu tại một thời điểm
                }
                
                // Kiểm tra tín hiệu đăng ký
                const registerRes = await fetch("/api/face-recognition/signal/pending-register");
                const registerData = await registerRes.json();

                if (registerData.success && registerData.mssv) {
                    console.log("📥 Nhận tín hiệu đăng ký từ ESP32 cho MSSV:", registerData.mssv);
                    this.stopSignalPolling(); // ✅ Dừng signal polling trước
                    this.stopAllModes();                                 // ❗ reset các trạng thái cũ
                    this.startRegisterMode(registerData.mssv);           // ✅ Gọi và truyền đúng MSSV
                    this.showNotification(`Nhận yêu cầu đăng ký cho MSSV: ${registerData.mssv}`, 'info');
                    return;
                }


                
            } catch (error) {
                console.error("Lỗi polling tín hiệu:", error);
                if (!this.lastPollingError || Date.now() - this.lastPollingError > 30000) {
                    console.warn("Lỗi kết nối polling tín hiệu, sẽ thử lại...");
                    this.lastPollingError = Date.now();
                }
            }
        }, 1500); // Kiểm tra tín hiệu mỗi 1.5 giây
    }

    stopSignalPolling() {
        if (this.signalPollingInterval) {
            clearInterval(this.signalPollingInterval);
            this.signalPollingInterval = null;
            console.log('🛑 Đã dừng polling tín hiệu');
        }
    }

    // Debug method để kiểm tra trạng thái polling
    debugPollingStatus() {
        console.log('🔍 Trạng thái polling:', {
            isSignalPolling: !!this.signalPollingInterval,
            isAttendanceMode: this.isAttendanceMode,
            isRegisterMode: this.isRegisterMode,
            pendingMSSV: this.pendingRegisterMSSV,
            isProcessing: this.isProcessingRegistration
        });
    }

    // Method để force restart signal polling từ console (dành cho debug)
    forceRestartPolling() {
        console.log('🔧 Force restart signal polling...');
        this.stopSignalPolling();
        setTimeout(() => {
            this.startSignalPolling();  
        }, 1000);
    }

    // Method để force reconnect stream từ console (dành cho debug)
    forceReconnectStream() {
        console.log('🔧 Force reconnect stream...');
        this.isStreamConnected = false;
        this.streamReconnectAttempts = 0;
        this.startAutoStream();
    }

    // Cleanup method
    cleanup() {
        console.log('🧹 Dọn dẹp hệ thống...');
        
        // Stop all intervals
        this.stopAllModes();
        this.stopSignalPolling();
        
        if (this.streamCheckInterval) {
            clearInterval(this.streamCheckInterval);
            this.streamCheckInterval = null;
        }
        
        // Reset stream status
        this.isStreamConnected = false;
        this.streamReconnectAttempts = 0;
        
        console.log('✅ Đã dọn dẹp hệ thống');
    }

    // ===== AUTO STREAM METHODS =====

    // Khởi tạo hệ thống tự động
    async initializeAutoSystem() {
        console.log('🚀 Khởi tạo hệ thống tự động...');
        this.updateStreamStatus('connecting', 'Đang khởi tạo hệ thống...');
        
        // Đợi face-api.js load
        await this.waitForFaceAPI();
        
        // Khởi động stream tự động
        await this.startAutoStream();
        await updateTodayStatistics();

        
        // Bắt đầu signal polling
        this.startSignalPolling();
        
        console.log('✅ Hệ thống tự động đã sẵn sàng');
    }

    // Đợi face-api.js load xong
    async waitForFaceAPI() {
        return new Promise((resolve) => {
            const checkFaceAPI = () => {
                if (typeof faceapi !== 'undefined' && faceapi.nets) {
                    console.log('✅ Face-api.js đã sẵn sàng');
                    resolve();
                } else {
                    console.log('⏳ Đang chờ face-api.js...');
                    setTimeout(checkFaceAPI, 500);
                }
            };
            checkFaceAPI();
        });
    }

    // Bắt đầu stream tự động
    // faceRecognition.js
    async startAutoStream() {
        console.log('📹 Bắt đầu stream tự động...');
        this.updateStreamStatus('connecting', 'Đang kết nối ESP32 stream...');
        
        // ✨ THAY ĐỔI: Thêm tham số ngẫu nhiên để tránh cache của trình duyệt
        const cacheBuster = `?_t=${new Date().getTime()}`;
        const streamUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/stream${cacheBuster}`;
        
        console.log(`🔄 Đang kết nối đến URL stream mới: ${streamUrl}`);

        // Set up stream
        this.esp32Stream.src = streamUrl;
        this.esp32Stream.crossOrigin = "anonymous";
        
        // Handle stream events
        this.setupStreamEventHandlers();
        
        // Start monitoring stream
        this.startStreamMonitoring();
    }

    // Setup stream event handlers
    setupStreamEventHandlers() {
        this.esp32Stream.onload = () => {
            console.log('✅ Stream kết nối thành công');
            this.isStreamConnected = true;
            this.streamReconnectAttempts = 0;
            this.updateStreamStatus('connected', '✅ Stream kết nối thành công');
            this.updateStatusIndicator('connected', 'Stream đang hoạt động');
        };

        this.esp32Stream.onerror = (error) => {
            console.error('❌ Lỗi stream:', error);
            this.isStreamConnected = false;
            this.handleStreamError();
        };

        this.esp32Stream.onabort = () => {
            console.warn('⚠️ Stream bị ngắt');
            this.isStreamConnected = false;
            this.handleStreamError();
        };
    }

    // Handle stream errors and reconnection
    handleStreamError() {
        this.streamReconnectAttempts++;
        
        if (this.streamReconnectAttempts <= this.maxReconnectAttempts) {
            console.log(`🔄 Thử kết nối lại stream (lần ${this.streamReconnectAttempts}/${this.maxReconnectAttempts})`);
            this.updateStreamStatus('connecting', `Đang thử kết nối lại (${this.streamReconnectAttempts}/${this.maxReconnectAttempts})...`);
            
            setTimeout(() => {
                this.startAutoStream();
            }, 2000 * this.streamReconnectAttempts); // Exponential backoff
        } else {
            console.error('❌ Không thể kết nối stream sau nhiều lần thử');
            this.updateStreamStatus('error', '❌ Không thể kết nối ESP32');
            this.updateStatusIndicator('error', 'Lỗi kết nối camera');
        }
    }

    // Monitor stream health
    startStreamMonitoring() {
        if (this.streamCheckInterval) {
            clearInterval(this.streamCheckInterval);
        }

        this.streamCheckInterval = setInterval(() => {
            this.checkStreamHealth();
        }, 10000); // Check every 10 seconds
    }

    // Check if stream is still healthy
    async checkStreamHealth() {
        if (!this.isStreamConnected) return;

        try {
            // Try to fetch a test capture to verify ESP32 is responsive
            const testUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/capture?test=1&ts=${Date.now()}`;
            const response = await fetch(testUrl, { 
                method: 'HEAD',
                timeout: 5000 
            });
            
            if (!response.ok) {
                throw new Error('ESP32 không phản hồi');
            }
        } catch (error) {
            console.warn('⚠️ Stream health check failed:', error.message);
            this.isStreamConnected = false;
            this.handleStreamError();
        }
    }

    // Update stream status display
    updateStreamStatus(status, message) {
        const streamStatusEl = document.getElementById('streamStatusText');
        const streamStatusContainer = document.getElementById('stream-status');
        
        if (streamStatusEl) {
            streamStatusEl.textContent = message;
        }
        
        if (streamStatusContainer) {
            streamStatusContainer.className = `stream-status ${status}`;
        }
    }

    // Update main status indicator
    updateStatusIndicator(status, message) {
        const statusIndicator = document.getElementById('status-indicator');
        const statusText = statusIndicator?.querySelector('.status-text');
        
        if (statusText) {
            statusText.textContent = message;
        }
        
        if (statusIndicator) {
            statusIndicator.className = `status-indicator ${status}`;
        }
    }

    // Ensure stream is connected before processing
    async ensureStreamConnection() {
        if (this.isStreamConnected) {
            return true;
        }

        console.log('🔄 Stream chưa kết nối, đang thử kết nối...');
        this.updateStreamStatus('connecting', 'Đang kết nối lại stream...');
        
        await this.startAutoStream();
        
        // Wait for connection or timeout
        return new Promise((resolve) => {
            const checkConnection = () => {
                if (this.isStreamConnected) {
                    resolve(true);
                } else if (this.streamReconnectAttempts >= this.maxReconnectAttempts) {
                    resolve(false);
                } else {
                    setTimeout(checkConnection, 1000);
                }
            };
            setTimeout(checkConnection, 2000); // Give it 2 seconds
        });
    }

    // Method để kiểm tra trạng thái hệ thống
    getSystemStatus() {
        return {
            isSignalPolling: !!this.signalPollingInterval,
            isAttendanceMode: this.isAttendanceMode,
            isRegisterMode: this.isRegisterMode,
            pendingMSSV: this.pendingRegisterMSSV,
            isProcessing: this.isProcessingRegistration,
            hasDescriptor: !!this.lastDescriptor,
            isStreamConnected: this.isStreamConnected,
            streamReconnectAttempts: this.streamReconnectAttempts
        };
    }

    startAutoRecognition() {
        console.log("🎯 Bắt đầu auto recognition (mặc định: điểm danh)");
        this.startAttendanceMode();
    }

    stopAutoRecognition() {
        console.log("⛔ Dừng auto recognition");
        this.stopAllModes();
    }
    async registerDescriptorWithMSSV(mssv) {
        try {
            console.log(`🔄 Đang đăng ký khuôn mặt cho MSSV: ${mssv}`);
            this.showNotificationDebounced(`Đang đăng ký khuôn mặt cho MSSV: ${mssv}`, 'info');

            const response = await fetch('/api/face-recognition/pending-register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    id: mssv,
                    descriptor: Array.from(this.lastDescriptor)
                })
            });

            const result = await response.json();
            if (result.success) {
                console.log(`✅ Đăng ký thành công cho MSSV: ${mssv}`);
                this.showNotification(`✅ Đăng ký khuôn mặt thành công cho MSSV ${mssv}`, 'success');
                this.addActivity("Đăng ký khuôn mặt", `Đăng ký thành công cho MSSV ${mssv}`, "success");

                // 🆕 Gửi tín hiệu cho ESP32 bắt đầu đăng ký vân tay
                try {
                    await fetch(`http://${this.esp32IpAddress}:81/start-fingerprint-register`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ mssv })
                    });
                    console.log(`📤 Đã gửi tín hiệu đăng ký vân tay cho ESP32`);
                } catch (fingerprintError) {
                    console.warn("⚠️ Không thể gửi tín hiệu đăng ký vân tay:", fingerprintError);
                }
                
                return true; // Trả về true khi thành công
            } else {
                console.error(`❌ Đăng ký thất bại cho MSSV ${mssv}:`, result.message);
                this.showNotification(`❌ Đăng ký thất bại: ${result.message}`, 'error');
                this.addActivity("Đăng ký thất bại", `MSSV ${mssv}: ${result.message}`, "error");
                return false;
            }
        } catch (error) {
            console.error('❌ Lỗi gửi descriptor:', error);
            this.showNotification('❌ Lỗi khi đăng ký khuôn mặt', 'error');
            this.addActivity("Lỗi đăng ký", `MSSV ${mssv}: ${error.message}`, "error");
            return false;
        }
    }

    

    async extractFaceDescriptor(imageElement) {
        try {
            // Cấu hình TinyFaceDetector với các tham số tối ưu
            const options = new faceapi.TinyFaceDetectorOptions({
                inputSize: 416,        // Kích thước input lớn hơn để phát hiện tốt hơn
                scoreThreshold: 0.3    // Ngưỡng điểm số thấp hơn để nhạy hơn
            });

            console.log('Đang phân tích ảnh...', {
                width: imageElement.width || imageElement.naturalWidth,
                height: imageElement.height || imageElement.naturalHeight,
                complete: imageElement.complete
            });

            const detections = await faceapi
                .detectSingleFace(imageElement, options)
                .withFaceLandmarks()
                .withFaceDescriptor();

            if (!detections) {
                // Thử với các cấu hình khác nếu không phát hiện được
                console.log('Không phát hiện được với cấu hình đầu tiên, thử cấu hình khác...');
                
                const alternativeOptions = new faceapi.TinyFaceDetectorOptions({
                    inputSize: 320,
                    scoreThreshold: 0.2
                });

                const alternativeDetections = await faceapi
                    .detectSingleFace(imageElement, alternativeOptions)
                    .withFaceLandmarks()
                    .withFaceDescriptor();

                if (!alternativeDetections) {
                    throw new Error("Không tìm thấy khuôn mặt nào trong ảnh.");
                }

                console.log('Phát hiện khuôn mặt thành công với cấu hình thay thế');
                return alternativeDetections.descriptor;
            }

            console.log('Phát hiện khuôn mặt thành công', {
                confidence: detections.detection.score,
                box: detections.detection.box
            });

            return detections.descriptor; // Trả về vector 128 chiều
        } catch (error) {
            console.error('Lỗi trong extractFaceDescriptor:', error);
            throw new Error(`Lỗi phân tích khuôn mặt: ${error.message}`);
        }
    }

    // Đảm bảo container thông báo đã được tạo
    ensureNotificationContainer() {
        if (!document.getElementById('notification-container')) {
            const container = document.createElement('div');
            container.id = 'notification-container';
            document.body.appendChild(container);
        }
    }
    
    hideLoadingScreen() {
        const loadingScreen = document.getElementById('loading-screen');
        if (loadingScreen) {
            loadingScreen.style.display = 'none';
        }
    }

    async init() {
        // Kiểm tra xem phần tử DOM tồn tại không
        if (!this.esp32Stream || !this.overlay) {
            console.error('Không tìm thấy phần tử cần thiết trong DOM!');
            return;
        }

        // this.setupEventListeners();

        try {
            // Load các models với error handling
            console.log('Đang tải models...');
            
            await faceapi.nets.tinyFaceDetector.loadFromUri('/models/tiny_face_detector');
            console.log('✓ TinyFaceDetector loaded');
            
            await faceapi.nets.faceLandmark68Net.loadFromUri('/models/face_landmark_68');
            console.log('✓ FaceLandmark68Net loaded');
            
            await faceapi.nets.faceRecognitionNet.loadFromUri('/models/face_recognition');
            console.log('✓ FaceRecognitionNet loaded');
            
            // await faceapi.nets.tinyYolov2.loadFromUri('/models/tiny_yolov2'); // ✅ THÊM DÒNG NÀY
            // console.log('✓ TinyYolov2 loaded');

            // Kiểm tra xem models đã được load đúng cách
            if (!faceapi.nets.tinyFaceDetector.isLoaded || 
                !faceapi.nets.faceLandmark68Net.isLoaded || 
                !faceapi.nets.faceRecognitionNet.isLoaded) {
                throw new Error('Một số models chưa được load đúng cách');
            }
            
            console.log("✓ Tất cả mô hình nhận diện khuôn mặt đã được load xong");

            // Kiểm tra database status
            await this.checkDatabaseStatus();

            // Hiển thị thông báo khởi động thành công
            this.showNotification('Hệ thống nhận diện khuôn mặt đã sẵn sàng', 'success');
            this.startSignalPolling();
            await updateTodayStatistics();

            
        } catch (error) {
            console.error('Lỗi khi load models:', error);
            this.showNotification('Lỗi khi tải models nhận diện khuôn mặt', 'error');
        }
    }

    setupEventListeners() {
        document.getElementById('startCamera').addEventListener('click', () => this.startCamera());
        document.getElementById('stopCamera').addEventListener('click', () => this.stopCamera());
        document.getElementById('capturePhoto').addEventListener('click', () => this.capturePhoto());
    }

    async startCamera() {
        try {
            // Kết nối với stream của ESP32
            const streamUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/stream`;
            
            // Hiển thị dialog để nhập IP của ESP32 (nếu cần)
            if (this.esp32IpAddress === '192.168.0.3') {
                const customIP = prompt('Nhập địa chỉ IP của ESP32 Camera (để trống để sử dụng mặc định: 192.168.0.3)');
                if (customIP && customIP.trim() !== '') {
                    this.esp32IpAddress = customIP.trim();
                    // Cập nhật URL stream
                    const newStreamUrl = `http://${this.esp32IpAddress}:${this.esp32StreamPort}/stream`;
                    this.showNotification(`Đã cập nhật IP: ${this.esp32IpAddress}`, 'info');
                    localStorage.setItem('esp32IpAddress', this.esp32IpAddress);
                }
            }
            
            // Lưu URL cũ để có thể so sánh
            const oldSrc = this.esp32Stream.src;
            this.esp32Stream.src = streamUrl;
            
            // Hiển thị thông báo đang kết nối
            this.showNotification(`Đang kết nối với ESP32 Camera (${this.esp32IpAddress})...`, 'info');
            this.updateStatus('connecting', 'Đang kết nối...');
            
            // Xử lý sự kiện khi ảnh tải thành công
            this.esp32Stream.onload = () => {
                this.isVideoPlaying = true;
                document.getElementById('startCamera').disabled = true;
                document.getElementById('stopCamera').disabled = false;
                document.getElementById('capturePhoto').disabled = false;
                this.updateStatus('connected', `Đã kết nối với ESP32 Camera (${this.esp32IpAddress})`);
                this.showNotification('Đã kết nối thành công với ESP32 Camera', 'success');
                
                // Thêm hoạt động vào danh sách
                this.addActivity('Kết nối camera', `Đã kết nối với ESP32 Camera (${this.esp32IpAddress})`, 'info');
                this.startAutoRecognition();

            };
            
            // Xử lý lỗi
            this.esp32Stream.onerror = () => {
                this.showNotification(`Không thể kết nối với ESP32 Camera (${this.esp32IpAddress})`, 'error');
                this.updateStatus('disconnected', 'Lỗi kết nối ESP32 Camera');
                
                // Thử lại với URL cũ nếu là lần đầu kết nối thất bại
                if (oldSrc && oldSrc !== streamUrl) {
                    this.showNotification('Đang thử lại với kết nối trước đó...', 'info');
                    this.esp32Stream.src = oldSrc;
                } else {
                    // Hiển thị gợi ý kiểm tra kết nối
                    this.showNotification('Gợi ý: Kiểm tra xem ESP32 Camera đã được bật và kết nối cùng mạng WiFi chưa', 'warning');
                    this.stopCamera();
                }
            };

        } catch (error) {
            console.error('Error accessing ESP32 camera:', error);
            this.showNotification('Không thể truy cập ESP32 camera', 'error');
            this.updateStatus('disconnected', 'Lỗi kết nối ESP32 Camera');
        }
    }

    stopCamera() {
        // Dừng tất cả các hoạt động liên quan trước
        this.stopAutoRecognition();
        this.stopSignalPolling();
        
        if (this.esp32Stream) {
            // Xóa event listeners để tránh trigger thêm events
            this.esp32Stream.onload = null;
            this.esp32Stream.onerror = null;
            
            // Ngưng stream từ ESP32
            const oldSrc = this.esp32Stream.src;
            this.esp32Stream.src = '';
            
            // Lưu URL cũ vào localStorage để lần sau có thể kết nối lại
            if (oldSrc) {
                localStorage.setItem('lastEsp32StreamUrl', oldSrc);
            }
        }

        this.isVideoPlaying = false;
        this.pendingRegisterMSSV = null; // Reset pending registration

        document.getElementById('startCamera').disabled = false;
        document.getElementById('stopCamera').disabled = true;
        document.getElementById('capturePhoto').disabled = true;

        this.updateStatus('disconnected', 'Đã ngắt kết nối ESP32 Camera');
        this.clearCanvas();
        
        // Hiển thị một thông báo duy nhất với debouncing
        this.showNotificationDebounced('Đã ngắt kết nối camera', 'info');
        this.addActivity('Ngắt kết nối camera', 'Đã ngắt kết nối ESP32 Camera', 'info');
    }

    capturePhoto() {
        // Tạo canvas có kích thước giống như ảnh ESP32 stream
        this.canvas.width = this.esp32Stream.width || 640;
        this.canvas.height = this.esp32Stream.height || 480;
        
        // Vẽ hình ảnh hiện tại từ ESP32 stream vào canvas
        this.context.drawImage(this.esp32Stream, 0, 0, this.canvas.width, this.canvas.height);

        const imageData = this.canvas.toDataURL('image/jpeg');

        // Gửi ảnh về server để xử lý khuôn mặt
        this.sendImageToServer(imageData);
        
        // Hiển thị thông báo đã chụp ảnh
        this.showNotification('Đã chụp ảnh từ ESP32 Camera', 'info');
    }

    async sendImageToServer(imageData) {
        try {
            const response = await fetch('/api/recognize', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ image: imageData })
            });

            const result = await response.json();
            if (result.success) {
                this.showNotification(`Xin chào ${result.name}`, 'success');
            } else {
                this.showNotification('Không nhận diện được khuôn mặt', 'warning');
            }
        } catch (err) {
            console.error('Error sending image:', err);
            this.showNotification('Lỗi gửi ảnh về server', 'error');
        }
    }

    updateStatus(status, message) {
        const statusElement = document.getElementById('status-indicator');
        if (statusElement) {
            const statusTextElement = statusElement.querySelector('.status-text');
            if (statusTextElement) {
                statusTextElement.textContent = message;
            }
            statusElement.className = `status-indicator ${status}`; // dùng class 'connected', 'disconnected', etc.
        }
    }

    showNotification(message, type = 'info') {
        // Tạo phần tử thông báo
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;
        
        // Thêm vào container
        const container = document.getElementById('notification-container');
        if (container) {
            container.appendChild(notification);
            
            // Tự động xóa sau 3 giây
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.classList.add('fadeOut');
                    setTimeout(() => {
                        if (notification.parentNode) {
                            container.removeChild(notification);
                        }
                    }, 500);
                }
            }, 3000);
        } else {
            console.warn('Không tìm thấy notification-container');
            alert(message);
        }
    }

    showNotificationDebounced(message, type = 'info', debounceTime = 1000) {
        const key = `${message}-${type}`;
        const now = Date.now();
        
        // Nếu thông báo giống nhau đã được hiển thị gần đây, bỏ qua
        if (this.lastNotification && this.lastNotification.key === key && 
            (now - this.lastNotification.time) < debounceTime) {
            return;
        }
        
        this.lastNotification = { key, time: now };
        this.showNotification(message, type);
    }

    clearAllNotifications() {
        const container = document.getElementById('notification-container');
        if (container) {
            container.innerHTML = '';
        }
    }

    // Method để dọn dẹp toàn bộ system khi cần reset
    cleanup() {
        console.log('Cleaning up Face Recognition System...');
        
        // Dừng tất cả intervals
        this.stopAutoRecognition();
        this.stopSignalPolling();
        
        // Reset tất cả các biến tracking
        this.isVideoPlaying = false;
        this.pendingRegisterMSSV = null;
        this.lastDescriptor = null;
        this.lastNotification = null;
        this.lastErrorNotification = 0;
        this.lastConnectionErrorNotification = 0;
        this.lastActivityKey = null;
        this.lastActivityTime = 0;
        this.isProcessingRegistration = false;
        this.lastPollingError = 0;
        
        // Clear notification queue
        this.notificationQueue = [];
        
        // Clear tất cả thông báo hiện tại
        this.clearAllNotifications();
        
        // Reset ESP32 stream
        if (this.esp32Stream) {
            this.esp32Stream.onload = null;
            this.esp32Stream.onerror = null;
            this.esp32Stream.src = '';
        }
        
        // Reset UI elements
        const startBtn = document.getElementById('startCamera');
        const stopBtn = document.getElementById('stopCamera');
        const captureBtn = document.getElementById('capturePhoto');
        
        if (startBtn) startBtn.disabled = false;
        if (stopBtn) stopBtn.disabled = true;
        if (captureBtn) captureBtn.disabled = true;
        
        this.updateStatus('disconnected', 'Hệ thống đã được reset');
        console.log('Face Recognition System cleaned up successfully');
    }

    clearCanvas() {
        this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }

    // Gửi descriptor lên server để nhận diện
    async sendDescriptorToServer(descriptor) {
        try {
            const response = await fetch('/api/face-recognition/recognize', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ 
                    descriptor: Array.from(descriptor)
                })
            });

            const result = await response.json();
            
            if (result.success) {
                this.showNotification(`Xin chào ${result.name}! (${result.className})`, 'success');
                this.addActivity("Nhận diện thành công", `${result.name} - ${result.className} (Distance: ${result.distance.toFixed(3)})`, "success");
                
                // Ghi nhận điểm danh
                await this.recordAttendance(result);
                
                return result;
            } else {
                console.log('Không nhận diện được người dùng:', result.message);
                this.addActivity("Không nhận diện được", result.message, "warning");
                return null;
            }
        } catch (error) {
            console.error('Lỗi gửi descriptor lên server:', error);
            this.addActivity("Lỗi server", "Không thể kết nối với server", "error");
            throw error;
        }
    }

    // Ghi nhận điểm danh
    async recordAttendance(recognitionResult) {
        try {
            const attendanceData = {
                studentInfo: {
                    id: recognitionResult.studentId,
                    name: recognitionResult.name
                },
                className: recognitionResult.className,
                timestamp: new Date().toISOString(),
                confidence: 1 - recognitionResult.distance // Chuyển distance thành confidence
            };

            const response = await fetch('/api/face-recognition/attendance', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(attendanceData)
            });

            const result = await response.json();
            
            if (result.success) {
                this.addActivity("Điểm danh thành công", `Đã ghi nhận điểm danh cho ${recognitionResult.name}`, "success");
            } else {
                console.log('Lỗi ghi điểm danh:', result.message);
                if (result.message.includes('already recorded')) {
                    this.addActivity("Đã điểm danh", `${recognitionResult.name} đã điểm danh hôm nay`, "info");
                } else {
                    this.addActivity("Lỗi điểm danh", result.message, "warning");
                }
            }
        } catch (error) {
            console.error('Lỗi ghi điểm danh:', error);
            this.addActivity("Lỗi điểm danh", "Không thể ghi điểm danh", "error");
        }
    }

    // Kiểm tra số lượng face descriptors trong database
    async checkDatabaseStatus() {
        try {
            const response = await fetch('/api/face-recognition/descriptors');
            const result = await response.json();
            
            if (result.success) {
                console.log(`Database có ${result.count} face descriptors`);
                this.showNotification(`Database có ${result.count} khuôn mặt đã đăng ký`, 'info');
                return result.data;
            } else {
                console.log('Không thể lấy thông tin database');
                this.showNotification('Không thể kết nối database', 'warning');
                return [];
            }
        } catch (error) {
            console.error('Lỗi kiểm tra database:', error);
            this.showNotification('Lỗi kết nối database', 'error');
            return [];
        }
    }

    // Hàm test để kiểm tra việc nhận diện khuôn mặt
    async testFaceDetection() {
        try {
            const img = document.getElementById('esp32Snapshot');
            if (!img || !img.src) {
                throw new Error('Không có ảnh để test');
            }

            console.log('Đang test nhận diện khuôn mặt...');
            const descriptor = await this.extractFaceDescriptor(img);
            console.log('Test thành công! Descriptor length:', descriptor.length);
            this.showNotification('Test nhận diện khuôn mặt thành công!', 'success');
            return descriptor;
        } catch (error) {
            console.error('Test thất bại:', error);
            this.showNotification(`Test thất bại: ${error.message}`, 'error');
            throw error;
        }
    }
    
}

// Khởi động sau khi DOM sẵn sàng
// Thêm phương thức để cập nhật hoạt động gần đây
FaceRecognitionSystem.prototype.addActivity = function(title, description, type = 'info') {
    const activityList = document.getElementById('activityList');
    if (!activityList) return;
    
    // Tránh thêm hoạt động trùng lặp trong khoảng thời gian ngắn
    const activityKey = `${title}-${type}`;
    if (this.lastActivityKey === activityKey && 
        Date.now() - (this.lastActivityTime || 0) < 2000) {
        return;
    }
    
    this.lastActivityKey = activityKey;
    this.lastActivityTime = Date.now();
    
    // Xóa thông báo "Chưa có hoạt động nào" nếu có
    const noActivity = activityList.querySelector('.no-activity');
    if (noActivity) {
        activityList.removeChild(noActivity);
    }
    
    // Tạo phần tử hoạt động mới
    const activityItem = document.createElement('div');
    activityItem.className = 'activity-item';
    
    // Lấy thời gian hiện tại
    const now = new Date();
    const timeStr = now.getHours().toString().padStart(2, '0') + ':' + 
                   now.getMinutes().toString().padStart(2, '0');
    
    // Tạo HTML cho hoạt động
    activityItem.innerHTML = `
        <div class="activity-icon ${type}">
            <i class="fi fi-rr-${type === 'success' ? 'check' : (type === 'error' ? 'cross' : 'info')}"></i>
        </div>
        <div class="activity-content">
            <h6>${title}</h6>
            <p>${description}</p>
        </div>
        <span class="activity-time">${timeStr}</span>
    `;
    
    // Thêm vào đầu danh sách
    activityList.insertBefore(activityItem, activityList.firstChild);
    
    // Giới hạn số lượng hoạt động hiển thị (tối đa 8 để tránh làm lag UI)
    const activities = activityList.querySelectorAll('.activity-item');
    if (activities.length > 8) {
        activityList.removeChild(activities[activities.length - 1]);
    }
};

window.addEventListener('DOMContentLoaded', () => {
    const app = new FaceRecognitionSystem();
    
    // Expose app to global scope for debugging
    window.faceRecognitionSystem = app;

    app.init();
    // Thêm các hàm debug vào global scope
    window.debugPolling = () => app.debugPollingStatus();
    window.forceRestartPolling = () => app.forceRestartPolling();
    window.forceReconnectStream = () => app.forceReconnectStream();
    window.getSystemStatus = () => app.getSystemStatus();
    window.cleanupSystem = () => app.cleanup();
    
    console.log('🎉 Hệ thống nhận diện khuôn mặt tự động đã khởi động');
    console.log('💡 Sử dụng các lệnh sau để debug:');
    console.log('   - debugPolling(): Kiểm tra trạng thái polling');
    console.log('   - getSystemStatus(): Xem trạng thái hệ thống');
    console.log('   - forceRestartPolling(): Khởi động lại polling');
    console.log('   - forceReconnectStream(): Kết nối lại stream');
    
    // Cleanup khi người dùng rời khỏi trang
    window.addEventListener('beforeunload', () => {
        if (app) {
            app.cleanup();
        }
    });
    
    // Auto pause/resume khi tab bị ẩn/hiện
    document.addEventListener('visibilitychange', () => {
        if (document.hidden && app) {
            console.log('🔇 Tab bị ẩn, tạm dừng polling để tiết kiệm tài nguyên');
            app.stopSignalPolling();
        } else if (!document.hidden && app) {
            console.log('🔊 Tab hiển thị lại, khởi động lại polling');
            setTimeout(() => app.startSignalPolling(), 1000);
        }
    });
});
async function updateTodayStatistics() {
    try {
        const res = await fetch('/api/face-recognition/today');
        const data = await res.json();

        if (!data.success) throw new Error(data.message || 'Lỗi lấy thống kê hôm nay');

        const stats = data.data.stats || {};
        const recent = data.data.recentAttendance || [];

        // Cập nhật thống kê
        document.getElementById('totalRecognized').textContent = stats.uniqueStudents || 0;
        document.getElementById('totalAttended').textContent = stats.totalRecords || 0;
        document.getElementById('recognitionAccuracy').textContent = stats.averageConfidence
            ? `${(stats.averageConfidence * 100).toFixed(1)}%`
            : '0%';

        // Cập nhật kết quả nhận diện gần nhất
        const recognitionContainer = document.getElementById('recognitionResults');
        recognitionContainer.innerHTML = ''; // Xóa cũ

        if (recent.length === 0) {
            recognitionContainer.innerHTML = '<p class="no-results">Chưa có kết quả nhận diện</p>';
        } else {
            recent.forEach((record) => {
                const item = document.createElement('div');
                item.className = 'recognition-result-item';

                // ✅ Sửa lại: dùng đúng key trong record
                const name = record.student_name || 'Không rõ';
                const className = record.class_name || 'Không rõ lớp';
                const time = record.formatted_time || '--:--';

                item.innerHTML = `
                    <strong>${name}</strong><br>
                    <span>${className}</span><br>
                    <small>🕒 Lúc ${time}</small>
                `;

                recognitionContainer.appendChild(item);
            });
        }
    } catch (err) {
        console.error('❌ Lỗi khi cập nhật thống kê:', err);
    }
}
