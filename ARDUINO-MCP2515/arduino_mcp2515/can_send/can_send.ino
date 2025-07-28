#include "AA_MCP2515.h"

const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_500kbps;
const uint8_t CAN_PIN_CS = 10;
const int8_t CAN_PIN_INT = 2;
const uint32_t CAN_SPI_HZ = 2000000;
CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT, SPI, CAN_SPI_HZ);

CANController CAN(config);

uint8_t data[] = { 'h', 'e', 'l', 'l', 'o' };

void setup() {
  Serial.begin(115200);
  delay(1000);  // Allow time for serial monitor to start
  Serial.println("Initializing CAN...");

  while (CAN.begin(CANController::Mode::Normal) != CANController::OK) {
    Serial.println("[ERROR] CAN.begin() failed. Retrying in 1 second...");
    delay(1000);
  }
  Serial.println("[OK] CAN controller initialized in NORMAL mode.");
}

void loop() {
  // Create and send CAN frame
  CANFrame frame(0x100, data, sizeof(data));

  Serial.print("[INFO] Attempting to send CAN frame: ");
  for (uint8_t i = 0; i < sizeof(data); i++) {
    Serial.print("0x");
    Serial.print(data[i], HEX);
    Serial.print(i < sizeof(data) - 1 ? " " : "");
  }

  Serial.println();

  CANController::IOResult result = CAN.write(frame);

  if (result == CANController::IOResult::OK) {
    Serial.println("[OK] Frame sent successfully.");
  } else {
    Serial.print("[FAIL] Failed to send frame. Error code: ");
    Serial.println(static_cast<int>(result));
  }

  // Optional: echo back frame to serial (verifies what was sent)
  frame.print("Echoed TX Frame");



  delay(2000);
}
