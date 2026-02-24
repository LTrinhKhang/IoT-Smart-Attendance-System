#ifndef FINGERPRINT_STORAGE_H
#define FINGERPRINT_STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

#define MAX_FINGERPRINTS 127  // Giới hạn của cảm biến vân tay

/**
 * @brief Class quản lý mapping giữa MSSV và ID cảm biến
 */
class FingerprintStorage {
public:
    /**
     * @brief Khởi tạo storage
     */
    bool begin();
    
    /**
     * @brief Tìm ID cảm biến trống tiếp theo
     * @return uint8_t ID trống (1-127) hoặc 0 nếu đầy
     */
    uint8_t getNextAvailableID();
    
    /**
     * @brief Lưu mapping MSSV -> ID cảm biến
     * @param mssv Mã số sinh viên
     * @param sensorID ID trong cảm biến (1-127)
     * @return true nếu lưu thành công
     */
    bool saveMSSVMapping(uint32_t mssv, uint8_t sensorID);
    
    /**
     * @brief Tìm ID cảm biến từ MSSV
     * @param mssv Mã số sinh viên
     * @return uint8_t ID cảm biến (1-127) hoặc 0 nếu không tìm thấy
     */
    uint8_t getSensorIDFromMSSV(uint32_t mssv);
    
    /**
     * @brief Tìm MSSV từ ID cảm biến
     * @param sensorID ID trong cảm biến (1-127)
     * @return uint32_t MSSV hoặc 0 nếu không tìm thấy
     */
    uint32_t getMSSVFromSensorID(uint8_t sensorID);
    
    /**
     * @brief Xóa mapping của một MSSV
     * @param mssv Mã số sinh viên
     * @return true nếu xóa thành công
     */
    bool deleteMSSVMapping(uint32_t mssv);
    
    /**
     * @brief Xóa mapping của một ID cảm biến
     * @param sensorID ID trong cảm biến
     * @return true nếu xóa thành công
     */
    bool deleteSensorIDMapping(uint8_t sensorID);
    
    /**
     * @brief Kiểm tra MSSV đã được đăng ký chưa
     * @param mssv Mã số sinh viên
     * @return true nếu đã đăng ký
     */
    bool isMSSVRegistered(uint32_t mssv);
    
    /**
     * @brief Lấy số lượng vân tay đã đăng ký
     * @return uint16_t Số lượng vân tay
     */
    uint16_t getRegisteredCount();
    
    /**
     * @brief In danh sách mapping (debug)
     */
    void printMappings();
    
    /**
     * @brief Xóa tất cả mapping (reset)
     */
    void clearAllMappings();

private:
    Preferences prefs;
    
    /**
     * @brief Tạo key cho MSSV mapping
     * @param mssv Mã số sinh viên
     * @return String Key để lưu trong Preferences
     */
    String getMSSVKey(uint32_t mssv);
    
    /**
     * @brief Tạo key cho sensor ID mapping
     * @param sensorID ID cảm biến
     * @return String Key để lưu trong Preferences
     */
    String getSensorKey(uint8_t sensorID);
};

// Global instance
extern FingerprintStorage fpStorage;

#endif