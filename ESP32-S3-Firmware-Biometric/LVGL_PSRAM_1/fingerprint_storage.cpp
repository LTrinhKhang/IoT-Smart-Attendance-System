#include "fingerprint_storage.h"

// Global instance
FingerprintStorage fpStorage;

bool FingerprintStorage::begin() {
    return prefs.begin("fp_storage", false);
}

uint8_t FingerprintStorage::getNextAvailableID() {
    // Tìm ID trống đầu tiên từ 1 đến 127
    for (uint8_t id = 1; id <= MAX_FINGERPRINTS; id++) {
        String key = getSensorKey(id);
        if (!prefs.isKey(key.c_str())) {
            return id;
        }
    }
    return 0; // Không có ID trống
}

bool FingerprintStorage::saveMSSVMapping(uint32_t mssv, uint8_t sensorID) {
    if (sensorID == 0 || sensorID > MAX_FINGERPRINTS) {
        Serial.printf("❌ ID cảm biến không hợp lệ: %d\n", sensorID);
        return false;
    }
    
    // Kiểm tra xem MSSV đã được đăng ký chưa
    if (isMSSVRegistered(mssv)) {
        Serial.printf("⚠️ MSSV %d đã được đăng ký\n", mssv);
        return false;
    }
    
    // Kiểm tra xem ID cảm biến đã được sử dụng chưa
    String sensorKey = getSensorKey(sensorID);
    if (prefs.isKey(sensorKey.c_str())) {
        Serial.printf("⚠️ ID cảm biến %d đã được sử dụng\n", sensorID);
        return false;
    }
    
    // Lưu mapping hai chiều
    String mssvKey = getMSSVKey(mssv);
    
    size_t written1 = prefs.putUChar(mssvKey.c_str(), sensorID);
    size_t written2 = prefs.putULong(sensorKey.c_str(), mssv);
    
    if (written1 > 0 && written2 > 0) {
        Serial.printf("✅ Đã lưu mapping: MSSV %d -> ID %d\n", mssv, sensorID);
        return true;
    } else {
        Serial.printf("❌ Lỗi lưu mapping: MSSV %d -> ID %d\n", mssv, sensorID);
        return false;
    }
}

uint8_t FingerprintStorage::getSensorIDFromMSSV(uint32_t mssv) {
    String key = getMSSVKey(mssv);
    return prefs.getUChar(key.c_str(), 0);
}

uint32_t FingerprintStorage::getMSSVFromSensorID(uint8_t sensorID) {
    String key = getSensorKey(sensorID);
    return prefs.getULong(key.c_str(), 0);
}

bool FingerprintStorage::deleteMSSVMapping(uint32_t mssv) {
    uint8_t sensorID = getSensorIDFromMSSV(mssv);
    if (sensorID == 0) {
        Serial.printf("⚠️ MSSV %d không tồn tại trong mapping\n", mssv);
        return false;
    }
    
    String mssvKey = getMSSVKey(mssv);
    String sensorKey = getSensorKey(sensorID);
    
    bool removed1 = prefs.remove(mssvKey.c_str());
    bool removed2 = prefs.remove(sensorKey.c_str());
    
    if (removed1 && removed2) {
        Serial.printf("✅ Đã xóa mapping: MSSV %d -> ID %d\n", mssv, sensorID);
        return true;
    } else {
        Serial.printf("❌ Lỗi xóa mapping: MSSV %d -> ID %d\n", mssv, sensorID);
        return false;
    }
}

bool FingerprintStorage::deleteSensorIDMapping(uint8_t sensorID) {
    uint32_t mssv = getMSSVFromSensorID(sensorID);
    if (mssv == 0) {
        Serial.printf("⚠️ ID cảm biến %d không tồn tại trong mapping\n", sensorID);
        return false;
    }
    
    return deleteMSSVMapping(mssv);
}

bool FingerprintStorage::isMSSVRegistered(uint32_t mssv) {
    return getSensorIDFromMSSV(mssv) != 0;
}

uint16_t FingerprintStorage::getRegisteredCount() {
    uint16_t count = 0;
    for (uint8_t id = 1; id <= MAX_FINGERPRINTS; id++) {
        String key = getSensorKey(id);
        if (prefs.isKey(key.c_str())) {
            count++;
        }
    }
    return count;
}

void FingerprintStorage::printMappings() {
    Serial.println("=== DANH SÁCH MAPPING VÂN TAY ===");
    uint16_t count = 0;
    
    for (uint8_t id = 1; id <= MAX_FINGERPRINTS; id++) {
        uint32_t mssv = getMSSVFromSensorID(id);
        if (mssv != 0) {
            Serial.printf("ID %d -> MSSV %d\n", id, mssv);
            count++;
        }
    }
    
    Serial.printf("Tổng cộng: %d vân tay đã đăng ký\n", count);
    Serial.println("================================");
}

void FingerprintStorage::clearAllMappings() {
    Serial.println("🗑️ Xóa tất cả mapping...");
    prefs.clear();
    Serial.println("✅ Đã xóa tất cả mapping");
}

String FingerprintStorage::getMSSVKey(uint32_t mssv) {
    return "mssv_" + String(mssv);
}

String FingerprintStorage::getSensorKey(uint8_t sensorID) {
    return "sid_" + String(sensorID);
}