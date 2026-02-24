#ifndef FINGER_H
#define FINGER_H

#include <Adafruit_Fingerprint.h>
#include <functional>
#include "fingerprint_storage.h"

// Default pin definitions for fingerprint sensor
#define FP_RX_PIN 48
#define FP_TX_PIN 47
#define FP_DEFAULT_BAUD 57600
#define FP_TIMEOUT_MS 10000  // 10 seconds timeout for operations

// Error codes
#define FP_ERROR_NONE 0
#define FP_ERROR_TIMEOUT 1
#define FP_ERROR_COMMUNICATION 2
#define FP_ERROR_IMAGE 3
#define FP_ERROR_FEATURE 4
#define FP_ERROR_MISMATCH 5
#define FP_ERROR_STORAGE 6
#define FP_ERROR_NOT_FOUND 7

// Callback function types
typedef std::function<void(int, const char*)> FingerprintCallback;

/**
 * @brief Class for managing fingerprint sensor operations
 * 
 * This class provides an interface to the fingerprint sensor for operations
 * such as enrollment, verification, and deletion of fingerprint templates.
 */
class FingerprintModule {
  public:
    /**
     * @brief Construct a new Fingerprint Module object
     * 
     * @param serial Hardware serial port to use for communication
     * @param rxPin RX pin number
     * @param txPin TX pin number
     * @param baud Baud rate for communication (default: 57600)
     */
    FingerprintModule(HardwareSerial* serial, uint8_t rxPin, uint8_t txPin, uint32_t baud = FP_DEFAULT_BAUD);
    
    /**
     * @brief Initialize the fingerprint sensor
     * 
     * @return true if sensor is detected and password verified
     * @return false if sensor initialization failed
     */
    bool begin();
    
    /**
     * @brief Enroll a new fingerprint
     * 
     * @param id ID to assign to the fingerprint (1-127)
     * @param statusCallback Callback function for status updates
     * @return int8_t 0 if successful, error code otherwise
     */
    int8_t enrollFingerprint(uint32_t mssv, FingerprintCallback statusCallback);
    
    /**
     * @brief Start the enrollment process with user input for ID
     * 
     * @param statusCallback Callback function for status updates
     * @return int8_t ID of enrolled fingerprint or negative error code
     */
    int8_t startEnrollment(FingerprintCallback statusCallback = nullptr);
    
    /**
     * @brief Check for a fingerprint match
     * 
     * @param timeout_ms Maximum time to wait for finger (in milliseconds)
     * @param statusCallback Callback function for status updates
     * @return int16_t ID of matched fingerprint or negative error code
     */
    int16_t checkFingerprint(uint32_t timeout_ms = FP_TIMEOUT_MS, 
                            FingerprintCallback statusCallback = nullptr);
    
    /**
     * @brief Delete a fingerprint from the database by sensor ID
     * 
     * @param id ID of fingerprint to delete
     * @param statusCallback Callback function for status updates
     * @return int8_t 0 if successful, error code otherwise
     */
    int8_t deleteFingerprint(uint8_t id, FingerprintCallback statusCallback = nullptr);
    
    /**
     * @brief Delete a fingerprint from the database by MSSV
     * 
     * @param mssv MSSV of student to delete
     * @param statusCallback Callback function for status updates
     * @return int8_t 0 if successful, error code otherwise
     */
    int8_t deleteFingerprintByMSSV(uint32_t mssv, FingerprintCallback statusCallback = nullptr);
    
    /**
     * @brief Get the number of stored templates
     * 
     * @return uint16_t Number of templates stored in sensor
     */
    uint16_t getTemplateCount();
    
    /**
     * @brief Get the last matched fingerprint confidence score
     * 
     * @return uint16_t Confidence score (0-255)
     */
    uint16_t getLastConfidence() const { return lastConfidence; }
    
    /**
     * @brief Get error message for error code
     * 
     * @param errorCode Error code
     * @return const char* Error message
     */
    static const char* getErrorMessage(int8_t errorCode);
    
    /**
     * @brief Print all registered fingerprints with MSSV mapping
     */
    void printRegisteredFingerprints();
    
    /**
     * @brief Get MSSV from sensor ID
     * 
     * @param sensorID ID in sensor (1-127)
     * @return uint32_t MSSV or 0 if not found
     */
    uint32_t getMSSVFromSensorID(uint8_t sensorID);
    
    /**
     * @brief Get sensor ID from MSSV
     * 
     * @param mssv Student ID
     * @return uint8_t Sensor ID (1-127) or 0 if not found
     */
    uint8_t getSensorIDFromMSSV(uint32_t mssv);
    
    /**
     * @brief Clear all fingerprint data and mappings
     */
    void clearAllData();

    bool getFingerprintTemplateBase64(String &outBase64);

    bool getTemplateBase64(String &outBase64, uint8_t id);
        /**
     * @brief Gửi lệnh upChar(1) thủ công
     */
    void sendUpCharCommand();
    bool sendDownCharCommand(uint8_t bufferID);
    bool set_verification_target_template(uint8_t* data, size_t length);
    bool isMSSVRegistered(uint32_t mssv);
    bool verifyAgainstTemplate(uint32_t timeout_ms = FP_TIMEOUT_MS, FingerprintCallback statusCallback = nullptr);
    int verifyAgainstMSSV(uint32_t mssv, uint32_t timeout_ms = FP_TIMEOUT_MS, FingerprintCallback statusCallback = nullptr);



  private:
    Adafruit_Fingerprint finger;
    HardwareSerial* hwSerial;
    uint8_t rx, tx;
    uint32_t baudrate;
    uint16_t lastConfidence;
    
    /**
     * @brief Internal function to enroll a fingerprint
     * 
     * @param id ID to assign to the fingerprint
     * @param statusCallback Callback function for status updates
     * @param timeout_ms Maximum time to wait for finger (in milliseconds)
     * @return int8_t 0 if successful, error code otherwise
     */
    int8_t getFingerprintEnroll(uint8_t id, FingerprintCallback statusCallback, 
                               uint32_t timeout_ms = FP_TIMEOUT_MS);
    
    /**
     * @brief Read a number from Serial input
     * 
     * @param timeout_ms Maximum time to wait for input (in milliseconds)
     * @return uint8_t Number read from Serial or 0 if timeout
     */
    uint8_t readNumber(uint32_t timeout_ms = 50000);
    
    /**
     * @brief Wait for finger to be placed on sensor
     * 
     * @param statusCallback Callback function for status updates
     * @param timeout_ms Maximum time to wait (in milliseconds)
     * @return true if finger detected within timeout
     * @return false if timeout occurred
     */
    bool waitForFinger(FingerprintCallback statusCallback, uint32_t timeout_ms);
    
    /**
     * @brief Wait for finger to be removed from sensor
     * 
     * @param statusCallback Callback function for status updates
     * @param timeout_ms Maximum time to wait (in milliseconds)
     * @return true if finger removed within timeout
     * @return false if timeout occurred
     */
    bool waitForNoFinger(FingerprintCallback statusCallback, uint32_t timeout_ms);

    
};

// Create a global instance of the fingerprint module
extern HardwareSerial FPSerial;
extern FingerprintModule fingerprint;

extern int decode_base64(const String& base64Str, uint8_t* outputBuffer);

// Hàm nạp template vào cảm biến




#endif
