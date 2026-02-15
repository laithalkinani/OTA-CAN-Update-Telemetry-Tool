#include "AA_MCP2515.h"

const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_500kbps;
const uint8_t CAN_PIN_CS = 10;
const int8_t CAN_PIN_INT = 2;
const uint32_t CAN_SPI_HZ = 2000000;
CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT, SPI, CAN_SPI_HZ);

CANController CAN(config);

uint8_t data[8] = {0};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing CAN...");

  while (CAN.begin(CANController::Mode::Normal) != CANController::OK) {
    Serial.println("[ERROR] CAN.begin() failed. Retrying...");
    delay(1000);
  }
  Serial.println("[OK] CAN initialized");
}

// Increment 64-bit counter (little-endian: data[0] is LSB)
void increment_counter() {
  for (uint8_t i = 0; i < 8; i++) {
    data[i]++;
    if (data[i] != 0) {
      break;  // No carry needed
    }
    // If data[i] wrapped to 0, carry to next byte
  }
}

void loop() {
  // Print current counter value
  Serial.print("TX [");
  for (int i = 7; i >= 0; i--) {  // Print MSB first
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    if (i > 0) Serial.print(" ");
  }
  Serial.print("]: ");

  // Create frame with CURRENT data
  CANFrame frame(0x100, data, sizeof(data));
  
  CANController::IOResult result = CAN.write(frame);
  
  if (result == CANController::IOResult::OK) {
    Serial.println("OK");
  } else {
    Serial.print("FAIL (");
    Serial.print(static_cast<int>(result));
    Serial.println(")");
    
    // Only print errors on failure
    CANErrors errs = CAN.getErrors();
    Serial.print("  TEC: ");
    Serial.print(errs.errorCountTx);
    Serial.print(" REC: ");
    Serial.println(errs.errorCountRx);
  }

  // Increment for next message
  increment_counter();
  
  // Check for rollover (back to all zeros)
  bool all_zero = true;
  for (uint8_t i = 0; i < 8; i++) {
    if (data[i] != 0) {
      all_zero = false;
      break;
    }
  }
  if (all_zero) {
    Serial.println("*** COUNTER ROLLED OVER (2^64 messages sent) ***");
    delay(5000);  // Pause to notice
  }
  
  // delay(100);  // 10 messages/sec = reasonable rate
}