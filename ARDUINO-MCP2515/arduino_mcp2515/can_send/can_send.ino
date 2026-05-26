#include "AA_MCP2515.h"

const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_500kbps;
const uint8_t  CAN_PIN_CS  = 10;
const int8_t   CAN_PIN_INT = 2;
const uint32_t CAN_SPI_HZ  = 2000000;
CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT, SPI, CAN_SPI_HZ);
CANController CAN(config);

uint8_t simVal = 0;  // 0–100, wraps

void packSignalBE(uint8_t* buf, uint8_t startBit, uint8_t bitLen, uint32_t rawVal) {
  for (int8_t i = bitLen - 1; i >= 0; i--) {
    uint8_t bitVal  = (rawVal >> (bitLen - 1 - i)) & 0x01;
    uint8_t dbcBit  = startBit - i;
    uint8_t byteIdx = dbcBit / 8;
    uint8_t bitIdx  = dbcBit % 8;
    if (bitVal) buf[byteIdx] |=  (1 << bitIdx);
    else        buf[byteIdx] &= ~(1 << bitIdx);
  }
}

void build_6B0(uint8_t* buf) {
  memset(buf, 0, 8);
  packSignalBE(buf,  7, 16, (uint32_t)simVal * 10);        // Pack_Current: 0–100 A
  packSignalBE(buf, 23, 16, 3000 + (uint32_t)simVal * 10); // Pack_Inst_Voltage: 300–400 V
  packSignalBE(buf, 39,  8, (uint32_t)simVal * 2);         // Pack_SOC: 0–100%
}

void build_6B1(uint8_t* buf) {
  memset(buf, 0, 8);
  uint8_t high_temp = 20 + (uint8_t)((simVal * 40UL) / 100); // High_Temp: 20–60 °C
  uint8_t low_temp  = 10 + (uint8_t)((simVal * 40UL) / 100); // Low_Temp:  10–50 °C
  packSignalBE(buf, 39, 8, high_temp);
  packSignalBE(buf, 47, 8, low_temp);
}

void sendFrame(uint16_t id, uint8_t* buf) {
  CANFrame frame(id, buf, 8);
  CANController::IOResult r = CAN.write(frame);

  Serial.print("0x"); Serial.print(id, HEX);
  Serial.print(" [");
  for (int i = 0; i < 8; i++) {
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    if (i < 7) Serial.print(" ");
  }
  Serial.print("] simVal="); Serial.print(simVal);
  Serial.println(r == CANController::IOResult::OK ? " OK" : " FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing CAN...");
  while (CAN.begin(CANController::Mode::Normal) != CANController::OK) {
    Serial.println("[ERROR] Retrying...");
    delay(1000);
  }
  Serial.println("[OK] CAN initialized");
}

void loop() {
  uint8_t buf[8];

  build_6B0(buf);
  sendFrame(0x6B0, buf);

  build_6B1(buf);
  sendFrame(0x6B1, buf);

  simVal++;
  if (simVal > 100) simVal = 0;

}