#include "Finger.h"
#include <base64.h>
#include "http_client.h"
#include "mbedtls/base64.h"
#include "globals.h"
#include "fingerprint_storage.h"


// Initialize the global instances
HardwareSerial FPSerial(2);
FingerprintModule fingerprint(&FPSerial, FP_RX_PIN, FP_TX_PIN);

#define FINGERPRINT_DATA    0x02
#define FINGERPRINT_ENDDATA 0x08
// #define FINGERPRINT_COMMANDPACKET 0x01
#define FINGERPRINT_STARTCODE 0xEF01


FingerprintModule::FingerprintModule(HardwareSerial* serial, uint8_t rxPin, uint8_t txPin, uint32_t baud)
  : finger(serial), hwSerial(serial), rx(rxPin), tx(txPin), baudrate(baud), lastConfidence(0) {}

bool FingerprintModule::begin() {
  // Initialize serial communication with the fingerprint sensor
  hwSerial->begin(baudrate, SERIAL_8N1, rx, tx);
  finger.begin(baudrate);
  
  // Initialize fingerprint storage
  if (!fpStorage.begin()) {
    Serial.println("⚠️ Không thể khởi tạo storage cho mapping vân tay");
  } else {
    Serial.println("✅ Storage mapping vân tay đã sẵn sàng");
    Serial.printf("📊 Số vân tay đã đăng ký: %d/127\n", fpStorage.getRegisteredCount());
  }
  
  // Verify the sensor is responding with correct password
  bool success = finger.verifyPassword();
  if (!success) {
    Serial.println("Không thể kết nối với cảm biến vân tay!");
    Serial.println("Kiểm tra lại kết nối và nguồn điện.");
  } else {
    Serial.println("Cảm biến vân tay đã sẵn sàng!");
  }
  return success;
}

int8_t FingerprintModule::startEnrollment(FingerprintCallback statusCallback) {
  if (statusCallback) {
    statusCallback(0, "Bắt đầu quá trình đăng ký vân tay mới");
    statusCallback(0, "Nhập ID từ 1 đến 127:");
  } else {
    Serial.println("=== ĐĂNG KÝ VÂN TAY MỚI ===");
    Serial.println("Nhập ID từ 1 đến 127:");
  }
  
  uint8_t id = readNumber();
  if (id == 0 || id > 127) {
    const char* msg = "ID không hợp lệ. Phải từ 1-127.";
    if (statusCallback) statusCallback(-1, msg);
    else Serial.println(msg);
    return -1;
  }
  
  return enrollFingerprint(id, statusCallback);
}

int8_t FingerprintModule::enrollFingerprint(uint32_t mssv, FingerprintCallback statusCallback) {
    uint8_t sensor_id;

    if (fpStorage.isMSSVRegistered(mssv)) {
        // Đã đăng ký → lấy lại ID cảm biến cũ
        sensor_id = fpStorage.getSensorIDFromMSSV(mssv);
        Serial.printf("♻️ MSSV %d đã tồn tại → ghi đè tại ID %d\n", mssv, sensor_id);

        // Xóa mẫu cũ trên cảm biến
        finger.deleteModel(sensor_id);

        // Xóa mapping cũ để ghi lại
        fpStorage.deleteMSSVMapping(mssv);
    } else {
        // MSSV chưa đăng ký → cấp ID mới theo thứ tự
        sensor_id = fpStorage.getNextAvailableID();
        if (sensor_id == 0) {
            if (statusCallback) statusCallback(-1, "Cảm biến đã đầy (127 vân tay)");
            Serial.println("❌ Cảm biến đã đầy");
            return -2;
        }
        Serial.printf("🆕 Tạo ID mới: %d cho MSSV: %d\n", sensor_id, mssv);
    }

    // Hiển thị thông báo
    if (statusCallback) {
        char msg[128];  // Tăng kích thước buffer
        snprintf(msg, sizeof(msg), "Dang ky MSSV %lu vao ID %d", (unsigned long)mssv, sensor_id);
        statusCallback(0, msg);
    }

    if (statusCallback) statusCallback(0, "🟡 Bắt đầu đăng ký vân tay...");

    Serial.println("📸 Chụp lần 1...");
    while (finger.getImage() != FINGERPRINT_OK);
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(-1, "Lỗi chuyển ảnh lần 1");
        return -3;
    }

    delay(500);
    Serial.println("📸 Chụp lần 2...");
    while (finger.getImage() != FINGERPRINT_OK);
    if (finger.image2Tz(2) != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(-1, "Lỗi chuyển ảnh lần 2");
        return -4;
    }

    Serial.println("🔄 Tạo model...");
    if (finger.createModel() != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(-1, "Tạo model thất bại");
        return -5;
    }

    Serial.printf("💾 Ghi model vào flash tại ID %d...\n", sensor_id);
    if (finger.storeModel(sensor_id) != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(-1, "Không lưu được mẫu");
        return -6;
    }

    // Ghi lại mapping
    if (!fpStorage.saveMSSVMapping(mssv, sensor_id)) {
        finger.deleteModel(sensor_id);
        if (statusCallback) statusCallback(-1, "Lỗi lưu mapping");
        return -7;
    }

    if (statusCallback) {
        char msg[128];  // Tăng kích thước buffer
        snprintf(msg, sizeof(msg), "Dang ky thanh cong! MSSV %lu -> ID %d", (unsigned long)mssv, sensor_id);
        statusCallback(sensor_id, msg);  // trả về ID cảm biến
    }

    Serial.printf("✅ Đăng ký thành công: MSSV %d -> ID cảm biến %d\n", mssv, sensor_id);
    Serial.printf("📊 Tổng số vân tay đã đăng ký: %d/127\n", fpStorage.getRegisteredCount());

    return sensor_id;
}



bool FingerprintModule::isMSSVRegistered(uint32_t mssv) {
    return fpStorage.isMSSVRegistered(mssv);  // gọi gián tiếp
}


int16_t FingerprintModule::checkFingerprint(uint32_t timeout_ms, FingerprintCallback statusCallback) {
  const char* msg = "Đặt vân tay để kiểm tra...";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  // Chờ người dùng đặt vân tay
  if (!waitForFinger(statusCallback, timeout_ms)) {
    msg = "Hết thời gian chờ.";
    if (statusCallback) statusCallback(FP_ERROR_TIMEOUT, msg);
    else Serial.println(msg);
    return -FP_ERROR_TIMEOUT;
  }
  
  // Chụp ảnh vân tay
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    msg = "Không thể chụp ảnh vân tay";
    if (statusCallback) statusCallback(FP_ERROR_IMAGE, msg);
    else Serial.println(msg);
    return -FP_ERROR_IMAGE;
  }
  
  // Chuyển ảnh sang template và lưu vào buffer 1
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    msg = "Không thể chuyển đổi ảnh thành mẫu";
    if (statusCallback) statusCallback(FP_ERROR_FEATURE, msg);
    else Serial.println(msg);
    return -FP_ERROR_FEATURE;
  }

  // Tìm kiếm vân tay trong cơ sở dữ liệu cảm biến
  if (statusCallback) statusCallback(0, "Đang tìm kiếm vân tay...");
  
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    uint8_t foundID = finger.fingerID;
    uint16_t confidence = finger.confidence;
    
    // Tìm MSSV từ ID cảm biến
    uint32_t mssv = fpStorage.getMSSVFromSensorID(foundID);
    
    if (mssv != 0) {
      char resultMsg[100];
      snprintf(resultMsg, sizeof(resultMsg), "✅ Tìm thấy MSSV: %d (độ tin cậy: %d)", mssv, confidence);
      if (statusCallback) statusCallback(0, resultMsg);
      else Serial.println(resultMsg);
      
      lastConfidence = confidence;
      return mssv;  // Trả về MSSV thay vì ID cảm biến
    } else {
      // ID cảm biến tồn tại nhưng không có trong mapping (dữ liệu không đồng bộ)
      char errorMsg[100];
      snprintf(errorMsg, sizeof(errorMsg), "⚠️ Tìm thấy vân tay ID %d nhưng không có MSSV tương ứng", foundID);
      if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, errorMsg);
      else Serial.println(errorMsg);
      return -FP_ERROR_NOT_FOUND;
    }
  } else if (p == FINGERPRINT_NOTFOUND) {
    msg = "Vân tay không có trong cơ sở dữ liệu";
    if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, msg);
    else Serial.println(msg);
    return -FP_ERROR_NOT_FOUND;
  } else {
    msg = "Lỗi khi tìm kiếm vân tay";
    if (statusCallback) statusCallback(FP_ERROR_COMMUNICATION, msg);
    else Serial.println(msg);
    return -FP_ERROR_COMMUNICATION;
  }
}


int8_t FingerprintModule::deleteFingerprint(uint8_t id, FingerprintCallback statusCallback) {
  if (id == 0 || id > 127) {
    const char* msg = "ID không hợp lệ. Phải từ 1-127.";
    if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, msg);
    else Serial.println(msg);
    return FP_ERROR_NOT_FOUND;
  }
  
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "Xóa ID #%d", id);
  
  if (statusCallback) statusCallback(0, buffer);
  else Serial.println(buffer);
  
  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    // Xóa mapping nếu có
    fpStorage.deleteSensorIDMapping(id);
    
    const char* msg = "Đã xóa mẫu vân tay thành công.";
    if (statusCallback) statusCallback(0, msg);
    else Serial.println(msg);
    return FP_ERROR_NONE;
  } else {
    int8_t errorCode = FP_ERROR_COMMUNICATION;
    const char* errorMsg;
    
    switch (p) {
      case FINGERPRINT_PACKETRECIEVEERR:
        errorMsg = "Lỗi giao tiếp";
        errorCode = FP_ERROR_COMMUNICATION;
        break;
      case FINGERPRINT_BADLOCATION:
        errorMsg = "ID không tồn tại";
        errorCode = FP_ERROR_NOT_FOUND;
        break;
      case FINGERPRINT_FLASHERR:
        errorMsg = "Lỗi bộ nhớ flash";
        errorCode = FP_ERROR_STORAGE;
        break;
      default:
        errorMsg = "Lỗi không xác định";
        errorCode = FP_ERROR_COMMUNICATION;
    }
    
    if (statusCallback) statusCallback(errorCode, errorMsg);
    else {
      Serial.print("Lỗi khi xóa: ");
      Serial.println(errorMsg);
    }
    return errorCode;
  }
}

int8_t FingerprintModule::deleteFingerprintByMSSV(uint32_t mssv, FingerprintCallback statusCallback) {
  // Tìm ID cảm biến từ MSSV
  uint8_t sensorID = fpStorage.getSensorIDFromMSSV(mssv);
  if (sensorID == 0) {
    char msg[100];
    snprintf(msg, sizeof(msg), "MSSV %d không tồn tại trong hệ thống", mssv);
    if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, msg);
    else Serial.println(msg);
    return FP_ERROR_NOT_FOUND;
  }
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "Xóa vân tay MSSV %d (ID cảm biến %d)", mssv, sensorID);
  
  if (statusCallback) statusCallback(0, buffer);
  else Serial.println(buffer);
  
  // Xóa vân tay khỏi cảm biến
  int p = finger.deleteModel(sensorID);
  if (p == FINGERPRINT_OK) {
    // Xóa mapping
    if (fpStorage.deleteMSSVMapping(mssv)) {
      char msg[100];
      snprintf(msg, sizeof(msg), "✅ Đã xóa vân tay MSSV %d thành công", mssv);
      if (statusCallback) statusCallback(0, msg);
      else Serial.println(msg);
      return FP_ERROR_NONE;
    } else {
      const char* msg = "⚠️ Xóa vân tay thành công nhưng lỗi xóa mapping";
      if (statusCallback) statusCallback(FP_ERROR_STORAGE, msg);
      else Serial.println(msg);
      return FP_ERROR_STORAGE;
    }
  } else {
    int8_t errorCode = FP_ERROR_COMMUNICATION;
    const char* errorMsg;
    
    switch (p) {
      case FINGERPRINT_PACKETRECIEVEERR:
        errorMsg = "Lỗi giao tiếp với cảm biến";
        errorCode = FP_ERROR_COMMUNICATION;
        break;
      case FINGERPRINT_BADLOCATION:
        errorMsg = "ID cảm biến không tồn tại";
        errorCode = FP_ERROR_NOT_FOUND;
        break;
      case FINGERPRINT_FLASHERR:
        errorMsg = "Lỗi bộ nhớ flash cảm biến";
        errorCode = FP_ERROR_STORAGE;
        break;
      default:
        errorMsg = "Lỗi không xác định khi xóa";
        errorCode = FP_ERROR_COMMUNICATION;
    }
    
    if (statusCallback) statusCallback(errorCode, errorMsg);
    else {
      Serial.print("Lỗi khi xóa vân tay MSSV: ");
      Serial.println(errorMsg);
    }
    return errorCode;
  }
}

uint16_t FingerprintModule::getTemplateCount() {
  uint8_t p = finger.getTemplateCount();
  if (p == FINGERPRINT_OK) {
    Serial.print("Tổng số mẫu đã lưu: ");
    Serial.println(finger.templateCount);
    return finger.templateCount;
  } else {
    Serial.println("Không thể đọc số lượng mẫu");
    return 0;
  }
}

const char* FingerprintModule::getErrorMessage(int8_t errorCode) {
  switch (errorCode) {
    case FP_ERROR_NONE:
      return "Không có lỗi";
    case FP_ERROR_TIMEOUT:
      return "Hết thời gian chờ";
    case FP_ERROR_COMMUNICATION:
      return "Lỗi giao tiếp với cảm biến";
    case FP_ERROR_IMAGE:
      return "Lỗi khi chụp ảnh vân tay";
    case FP_ERROR_FEATURE:
      return "Lỗi khi trích xuất đặc trưng";
    case FP_ERROR_MISMATCH:
      return "Hai lần quét không khớp";
    case FP_ERROR_STORAGE:
      return "Lỗi lưu trữ";
    case FP_ERROR_NOT_FOUND:
      return "Không tìm thấy vân tay";
    default:
      return "Lỗi không xác định";
  }
}

void FingerprintModule::printRegisteredFingerprints() {
  Serial.println("=== DANH SÁCH VÂN TAY ĐĂNG KÝ ===");
  
  // In thông tin từ cảm biến
  uint16_t sensorCount = getTemplateCount();
  Serial.printf("Số vân tay trong cảm biến: %d\n", sensorCount);
  
  // In thông tin từ mapping
  uint16_t mappingCount = fpStorage.getRegisteredCount();
  Serial.printf("Số mapping đã lưu: %d\n", mappingCount);
  
  // In chi tiết mapping
  fpStorage.printMappings();
  
  Serial.println("===================================");
}

uint32_t FingerprintModule::getMSSVFromSensorID(uint8_t sensorID) {
  return fpStorage.getMSSVFromSensorID(sensorID);
}

uint8_t FingerprintModule::getSensorIDFromMSSV(uint32_t mssv) {
  return fpStorage.getSensorIDFromMSSV(mssv);
}

void FingerprintModule::clearAllData() {
  Serial.println("🗑️ Xóa tất cả dữ liệu vân tay...");
  
  // Xóa tất cả vân tay khỏi cảm biến
  for (uint8_t id = 1; id <= 127; id++) {
    finger.deleteModel(id);
  }
  
  // Xóa tất cả mapping
  fpStorage.clearAllMappings();
  
  Serial.println("✅ Đã xóa tất cả dữ liệu vân tay và mapping");
}

uint8_t FingerprintModule::readNumber(uint32_t timeout_ms) {
  uint8_t num = 0;
  unsigned long startTime = millis();
  
  while (num == 0) {
    // Wait for serial input with timeout
    while (!Serial.available()) {
      if (millis() - startTime > timeout_ms) {
        Serial.println("Hết thời gian chờ nhập.");
        return 0;
      }
      delay(10);
    }
    num = Serial.parseInt();
  }
  return num;
}

bool FingerprintModule::waitForFinger(FingerprintCallback statusCallback, uint32_t timeout_ms) {
    uint32_t startTime = millis();

    // 1. Đảm bảo cảm biến đang không có tay
    if (statusCallback) statusCallback(0, "👆 Nhấc tay khỏi cảm biến...");
    while (true) {
        int p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) break;  // OK

        if (millis() - startTime > timeout_ms / 2) {
            if (statusCallback) statusCallback(FP_ERROR_TIMEOUT, "⌛ Quá lâu không nhấc tay khỏi cảm biến");
            return false;
        }

        delay(200);  // chậm lại để người dùng nhấc tay
    }

    delay(300);  // đệm thêm 300ms

    // 2. Bắt đầu chờ người dùng đặt tay mới
    if (statusCallback) statusCallback(0, "👉 Đặt tay vào cảm biến...");
    startTime = millis();
    bool fingerDetected = false;

    while (millis() - startTime < timeout_ms) {
        int p = finger.getImage();
        if (p == FINGERPRINT_OK) {
            Serial.println("[OK] Đã phát hiện ngón tay mới.");
            return true;
        }

        if (p != FINGERPRINT_NOFINGER) {
            if (statusCallback) statusCallback(FP_ERROR_COMMUNICATION, "❌ Lỗi cảm biến");
            return false;
        }

        delay(100);  // vừa phải để tránh trôi nhanh
    }

    if (statusCallback) statusCallback(FP_ERROR_TIMEOUT, "⌛ Hết thời gian chờ đặt tay");
    return false;
}


bool FingerprintModule::waitForNoFinger(FingerprintCallback statusCallback, uint32_t timeout_ms) {
  unsigned long startTime = millis();
  
  const char* msg = "Vui lòng bỏ ngón tay ra...";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    // Check for timeout
    if (millis() - startTime > timeout_ms) {
      msg = "Hết thời gian chờ bỏ ngón tay.";
      if (statusCallback) statusCallback(FP_ERROR_TIMEOUT, msg);
      else Serial.println(msg);
      return false;
    }
    delay(100);
  }
  
  return true;
}

int8_t FingerprintModule::getFingerprintEnroll(uint8_t id, FingerprintCallback statusCallback, uint32_t timeout_ms) {
  int p = -1;
  
  // First image capture
  const char* msg = "Đặt ngón tay lên cảm biến...";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  if (!waitForFinger(statusCallback, timeout_ms)) {
    return FP_ERROR_TIMEOUT;
  }
  
  p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    return FP_ERROR_IMAGE;
  }
  
  msg = "Ảnh đã chụp thành công.";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  // Convert first image to template
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    msg = "Không thể chuyển đổi ảnh thành mẫu";
    if (statusCallback) statusCallback(FP_ERROR_FEATURE, msg);
    else Serial.println(msg);
    return FP_ERROR_FEATURE;
  }
  
  // Wait for finger removal
  if (!waitForNoFinger(statusCallback, timeout_ms)) {
    return FP_ERROR_TIMEOUT;
  }
  
  delay(1000); // Short delay before second scan
  
  // Second image capture
  msg = "Đặt lại cùng ngón tay...";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  if (!waitForFinger(statusCallback, timeout_ms)) {
    return FP_ERROR_TIMEOUT;
  }
  
  p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    return FP_ERROR_IMAGE;
  }
  
  // Convert second image to template
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    msg = "Không thể chuyển đổi ảnh thứ hai thành mẫu";
    if (statusCallback) statusCallback(FP_ERROR_FEATURE, msg);
    else Serial.println(msg);
    return FP_ERROR_FEATURE;
  }
  
  // Create a model from the two templates
  msg = "Tạo mẫu vân tay...";
  if (statusCallback) statusCallback(0, msg);
  else Serial.println(msg);
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_ENROLLMISMATCH) {
      msg = "Hai lần quét không khớp. Vui lòng thử lại.";
      if (statusCallback) statusCallback(FP_ERROR_MISMATCH, msg);
      else Serial.println(msg);
      return FP_ERROR_MISMATCH;
    } else {
      msg = "Lỗi khi tạo mẫu";
      if (statusCallback) statusCallback(FP_ERROR_FEATURE, msg);
      else Serial.println(msg);
      return FP_ERROR_FEATURE;
    }
  }
  
  // Store the model
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "Lưu mẫu vào ID #%d", id);
  
  if (statusCallback) statusCallback(0, buffer);
  else Serial.println(buffer);
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    msg = "Đã lưu vân tay thành công!";
    if (statusCallback) statusCallback(0, msg);
    else Serial.println(msg);
    return FP_ERROR_NONE;
  } else {
    msg = "Lỗi khi lưu mẫu vân tay";
    if (statusCallback) statusCallback(FP_ERROR_STORAGE, msg);
    else Serial.println(msg);
    return FP_ERROR_STORAGE;
  }
}
bool FingerprintModule::getFingerprintTemplateBase64(String &outBase64) {
    // Gộp mẫu từ buffer1 + buffer2 thành 1 template
    int p = finger.getModel();
    if (p != FINGERPRINT_OK) {
        Serial.println(" Không thể tạo mẫu từ buffer 1 + 2");
        return false;
    }

    // Gửi lệnh để tải mẫu từ buffer 1
    p = finger.upChar(1);
    if (p != FINGERPRINT_OK) {
        Serial.println(" Lỗi khi upChar");
        return false;
    }

    // Đọc dữ liệu từ Serial (template raw ~512 byte)
    uint8_t tmpl[512];
    size_t count = 0;
    while (FPSerial.available() && count < sizeof(tmpl)) {
        tmpl[count++] = FPSerial.read();
    }

    if (count == 0) {
        Serial.println(" Không nhận được dữ liệu mẫu");
        return false;
    }

    outBase64 = base64::encode(tmpl, count);
    return true;
}

bool FingerprintModule::getTemplateBase64(String &output, uint8_t id) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
        Serial.println("❌ Không load được mẫu từ cảm biến trong getTemplateBase64()");
        return false;
    }

    delay(100);
    sendUpCharCommand();  // Gửi lệnh UpChar(1)

    const size_t maxRawSize = 600;
    uint8_t raw[maxRawSize] = {0};
    size_t count = 0;
    unsigned long start = millis();

    while (millis() - start < 3000 && count < maxRawSize) {
        while (FPSerial.available() && count < maxRawSize) {
            raw[count++] = FPSerial.read();
        }
        delay(1);
    }

    if (count < 32) {
        Serial.printf("❌ Dữ liệu quá ít từ cảm biến (%d byte)\n", (int)count);
        return false;
    }

    const size_t headerLen = 9;
    const size_t footerLen = 2;
    size_t dataLen = count - headerLen - footerLen;

    if (dataLen > 512) dataLen = 512; // Chỉ giữ 512 byte template
    if (dataLen == 0) {
        Serial.println("❌ Không có dữ liệu template hợp lệ");
        return false;
    }

    output = base64::encode(raw + headerLen, dataLen);

    Serial.printf("✅ Đã lấy template base64 (gốc %d byte, encoded %d byte)\n", (int)dataLen, output.length());
    return true;
}

void FingerprintModule::sendUpCharCommand() {
    uint8_t packet[13];

    packet[0] = 0xEF;
    packet[1] = 0x01;

    // Address (default: 0xFFFFFFFF)
    for (int i = 2; i <= 5; i++) packet[i] = 0xFF;

    packet[6] = 0x01;  // Packet type: command
    packet[7] = 0x00;
    packet[8] = 0x04;

    packet[9] = 0x08;  // upChar
    packet[10] = 0x01; // bufferID = 1

    uint16_t sum = 0x01 + 0x00 + 0x04 + 0x08 + 0x01;
    packet[11] = (sum >> 8) & 0xFF;
    packet[12] = sum & 0xFF;

    FPSerial.write(packet, 13);
    FPSerial.flush();
}

bool FingerprintModule::sendDownCharCommand(uint8_t bufferID) {
    uint8_t packet[13];

    packet[0] = 0xEF;
    packet[1] = 0x01;

    // Địa chỉ mặc định: 0xFFFFFFFF
    for (int i = 2; i <= 5; i++) packet[i] = 0xFF;

    packet[6] = 0x01;       // Command packet
    packet[7] = 0x00;
    packet[8] = 0x04;       // Length = 4 (command + 1 param + checksum)

    packet[9] = 0x09;       // 🔁 downChar
    packet[10] = bufferID;  // buffer 1 hoặc 2

    uint16_t sum = 0x01 + 0x00 + 0x04 + 0x09 + bufferID;
    packet[11] = (sum >> 8) & 0xFF;
    packet[12] = sum & 0xFF;

    FPSerial.write(packet, 13);
    FPSerial.flush();

    // ✅ Chờ ACK trả về (12 byte)
    uint8_t ack[12];
    unsigned long start = millis();
    while (millis() - start < 1000) {
        if (FPSerial.available() >= 12) {
            FPSerial.readBytes(ack, 12);
            if (ack[6] == 0x07 && ack[9] == 0x00) {
                Serial.println("✅ Cảm biến xác nhận downChar thành công");
                return true;
            } else {
                Serial.printf("❌ downChar bị từ chối. Mã lỗi: 0x%02X\n", ack[9]);
                return false;
            }
        }
        delay(10);
    }

    Serial.println("❌ Không nhận được ACK từ cảm biến sau downChar");
    return false;
}

int decode_base64(const String& base64Str, uint8_t* outputBuffer) {
    size_t outLen = 0;
    int ret = mbedtls_base64_decode(outputBuffer, 1024, &outLen,
                                    (const uint8_t*)base64Str.c_str(), base64Str.length());

    Serial.printf("➡️ Đã decode base64 được %d byte\n", (int)outLen);
    Serial.print("Raw decoded data (first 12 bytes): ");
    for (int i = 0; i < 12 && i < outLen; i++) {
        Serial.printf("%02X ", outputBuffer[i]);
    }
    Serial.println();


    if (ret != 0 || outLen != 512) {
        Serial.printf("❌ Lỗi giải mã hoặc không đúng 512 byte (got %d)\n", (int)outLen);
        return 0;
    }

    return (int)outLen;
}

bool FingerprintModule::set_verification_target_template(uint8_t* data, size_t length) {
    if (!data || length != 512) {
        Serial.printf("❌ Dữ liệu template không hợp lệ (length=%d)\n", (int)length);
        return false;
    }

    const uint8_t PACKET_SIZE = 128;
    const uint8_t BUFFER_ID = 0x02;

    // ===== 1. Gửi downChar để ghi vào buffer 2 =====
    uint8_t packet[13] = {
        0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,  // Header + address
        0x01, 0x00, 0x04,                    // Command packet, length 4
        0x09, BUFFER_ID,                    // downChar, buffer 2
        0x00, 0x00                          // checksum placeholder
    };

    uint16_t sum = 0x01 + 0x00 + 0x04 + 0x09 + BUFFER_ID;
    packet[11] = (sum >> 8) & 0xFF;
    packet[12] = sum & 0xFF;

    FPSerial.write(packet, 13);
    FPSerial.flush();
    delay(150);  // ⚠️ Tăng delay sau downChar

    // ===== 2. Đợi phản hồi ACK =====
    uint8_t ack[12] = {0};
    bool ackReceived = false;
    unsigned long start = millis();
    while (millis() - start < 1000) {
        if (FPSerial.available() >= 12) {
            FPSerial.readBytes(ack, 12);
            ackReceived = true;
            break;
        }
        delay(10);
    }

    if (!ackReceived) {
        Serial.println("❌ Không nhận được ACK từ cảm biến sau downChar (timeout)");
        return false;
    }

    if (!(ack[6] == 0x07 && ack[9] == 0x00)) {
        Serial.printf("❌ downChar thất bại (ACK code: 0x%02X)\n", ack[9]);
        return false;
    }

    Serial.println("✅ ACK downChar OK, chuẩn bị gửi template...");

    // ===== 3. Gửi 4 gói template (512 byte) =====
    for (uint8_t i = 0; i < 4; ++i) {
        uint8_t packetType = (i == 3) ? FINGERPRINT_ENDDATA : FINGERPRINT_DATA;
        uint8_t pkt[9 + PACKET_SIZE + 2] = {0};

        pkt[0] = 0xEF;
        pkt[1] = 0x01;
        for (int j = 2; j <= 5; ++j) pkt[j] = 0xFF;

        pkt[6] = packetType;
        pkt[7] = 0x00;
        pkt[8] = PACKET_SIZE + 2;

        memcpy(&pkt[9], data + i * PACKET_SIZE, PACKET_SIZE);

        uint16_t checksum = pkt[6] + pkt[7] + pkt[8];
        for (int j = 0; j < PACKET_SIZE; ++j)
            checksum += pkt[9 + j];

        pkt[9 + PACKET_SIZE] = (checksum >> 8) & 0xFF;
        pkt[10 + PACKET_SIZE] = checksum & 0xFF;

        Serial.printf("📤 Gửi gói %d (type=0x%02X)...\n", i + 1, packetType);
        Serial.printf("✅ Checksum gói %d: 0x%04X\n", i+1, checksum);

        FPSerial.write(pkt, sizeof(pkt));
        FPSerial.flush();
        delay(300);  // ⚠️ Tăng delay mỗi gói để đảm bảo cảm biến xử lý
    }

    // ===== 4. Đợi phản hồi cuối từ cảm biến =====
    uint8_t response[12] = {0};
    bool gotResponse = false;
    start = millis();
    while (millis() - start < 2000) {
        if (FPSerial.available() >= 12) {
            FPSerial.readBytes(response, 12);
            gotResponse = true;
            break;
        }
        delay(10);
    }

    if (!gotResponse) {
        Serial.println("❌ Không nhận được phản hồi cuối từ cảm biến (timeout)");
        return false;
    }

    Serial.print("📩 Phản hồi từ cảm biến: ");
    for (int i = 0; i < 12; ++i) Serial.printf("%02X ", response[i]);
    Serial.println();

    if (response[6] == 0x07 && response[9] == 0x00) {
        Serial.println("✅ Nạp template vào buffer 2 thành công");
        return true;
    } else {
        Serial.printf("❌ Lỗi nạp template (ACK code: 0x%02X)\n", response[9]);
        return false;
    }
}

// Hàm so sánh vân tay với MSSV cụ thể (sử dụng template đã lưu trong cảm biến)
int FingerprintModule::verifyAgainstMSSV(uint32_t mssv, uint32_t timeout_ms, FingerprintCallback statusCallback) {
    if (!isMSSVRegistered(mssv)) {
        const char* msg = "❌ MSSV chưa đăng ký vân tay";
        if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, msg);
        return FP_ERROR_NOT_FOUND;
    }

    uint8_t sensorID = fpStorage.getSensorIDFromMSSV(mssv);
    if (sensorID == 0) {
        const char* msg = "❌ Không tìm thấy sensor ID từ MSSV";
        if (statusCallback) statusCallback(FP_ERROR_NOT_FOUND, msg);
        return FP_ERROR_NOT_FOUND;
    }

    Serial.printf("🔍 Xác thực vân tay cho MSSV %u (sensor ID: %d)\n", mssv, sensorID);

    if (statusCallback) statusCallback(0, "📥 Đang tải mẫu đã đăng ký từ cảm biến...");
    int p = finger.loadModel(sensorID);
    if (p != FINGERPRINT_OK) {
        char errorMsg[64];
        snprintf(errorMsg, sizeof(errorMsg), "❌ Không thể tải mẫu ID %d (code: %d)", sensorID, p);
        if (statusCallback) statusCallback(FP_ERROR_STORAGE, errorMsg);
        return FP_ERROR_STORAGE;
    }

    const char* msg = "👉 Vui lòng đặt ngón tay để xác thực...";
    if (statusCallback) statusCallback(0, msg);
    if (!waitForFinger(statusCallback, timeout_ms)) {
        if (statusCallback) statusCallback(FP_ERROR_TIMEOUT, "⌛ Hết thời gian chờ vân tay");
        return FP_ERROR_TIMEOUT;
    }

    p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(FP_ERROR_IMAGE, "❌ Không thể chụp ảnh vân tay");
        return FP_ERROR_IMAGE;
    }

    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        if (statusCallback) statusCallback(FP_ERROR_FEATURE, "❌ Không thể chuyển ảnh thành template");
        return FP_ERROR_FEATURE;
    }

    if (statusCallback) statusCallback(0, "🔎 Đang so sánh vân tay...");
    p = finger.compareModel();

    if (p == FINGERPRINT_OK) {
        lastConfidence = finger.confidence;
        char resultMsg[100];
        snprintf(resultMsg, sizeof(resultMsg), "✅ Vân tay khớp với MSSV %u (độ tin cậy: %d)", mssv, lastConfidence);
        if (statusCallback) statusCallback(FP_ERROR_NONE, resultMsg);
        return FP_ERROR_NONE;
    }

    if (p == FINGERPRINT_NOMATCH) {
        char failMsg[96];
        snprintf(failMsg, sizeof(failMsg), "❌ Vân tay KHÔNG khớp với MSSV %u", mssv);
        if (statusCallback) statusCallback(FP_ERROR_MISMATCH, failMsg);
        return FP_ERROR_MISMATCH;
    }

    if (statusCallback) statusCallback(FP_ERROR_COMMUNICATION, "❌ Lỗi khi so sánh mẫu vân tay");
    return FP_ERROR_COMMUNICATION;
}










