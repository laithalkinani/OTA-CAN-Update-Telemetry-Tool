#include "AA_MCP2515.h"

#define ADC_MIN ((uint16_t)0)
#define ADC_MAX ((uint16_t)1023)
#define MAX_ERPM ((uint32_t)50000)
#define MIN_MOTOR_TEMP ((float)20.0)
#define MAX_MOTOR_TEMP ((float)90.0)
#define CURRENT_ALPHA ((float)0.15)   //arbitrary value for current delta inertia
#define TEMP_ALPHA    ((float)0.02)   //arbitrary value for temp delta inertia


const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_500kbps;
const uint8_t  CAN_PIN_CS  = 10;
const int8_t   CAN_PIN_INT = 2;
const uint32_t CAN_SPI_HZ  = 2000000;
CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT, SPI, CAN_SPI_HZ);
CANController CAN(config);

const uint8_t potPin = A0;
uint8_t throttlePct = 0;
float packCurrentPct = 0.0;
float tempPct = 0.0;



void packSignalBE(uint8_t* buf, uint8_t startBit, uint8_t bitLen, uint32_t rawVal) 
{
  for (int8_t i = bitLen - 1; i >= 0; i--) 
  {
    uint8_t bitVal  = (rawVal >> (bitLen - 1 - i)) & 0x01;
    uint8_t dbcBit  = startBit - i;
    uint8_t byteIdx = dbcBit / 8;
    uint8_t bitIdx  = dbcBit % 8;
    if (bitVal) buf[byteIdx] |=  (1 << bitIdx);
    else        buf[byteIdx] &= ~(1 << bitIdx);
  }
}

/*  Orion BMS CAN ID's*/

void build_6B0(uint8_t* buf, uint32_t input) 
{
  memset(buf, 0, 8);
  packSignalBE(buf,  7, 16, input * 10);        // Pack_Current: 0–100 A
  packSignalBE(buf, 23, 16, 3000 + input * 10); // Pack_Inst_Voltage: 300–400 V
  packSignalBE(buf, 39,  8, input * 2);         // Pack_SOC: 0–100%
}

void build_6B1(uint8_t* buf, uint8_t input) 
{
  memset(buf, 0, 8);
  uint8_t high_temp = 20 + (uint8_t)((input * 40UL) / 100); // High_Temp: 20–60 °C
  uint8_t low_temp  = 10 + (uint8_t)((input * 40UL) / 100); // Low_Temp:  10–50 °C
  packSignalBE(buf, 39, 8, high_temp);
  packSignalBE(buf, 47, 8, low_temp);
}

/*  DTI Inverter CAN ID's */

void build_494(uint8_t* buf, uint8_t throttle)
{
  memset(buf, 0, 8);
  packSignalBE(buf, 7, 8, throttle);      //throttle open: 0-100%
  packSignalBE(buf, 15, 8, 0);            //actual_brake: 0
  packSignalBE(buf, 31, 8, 1);            //drive_enable: 1 (enabled)

}

void sendFrame(uint16_t id, uint8_t* buf) 
{
  CANFrame frame(id, buf, 8);
  CANController::IOResult r = CAN.write(frame);

  Serial.print("0x"); Serial.print(id, HEX);
  Serial.print(" [");
  for (int i = 0; i < 8; i++) 
  {
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    if (i < 7) Serial.print(" ");
  }
  Serial.print("] throttle="); Serial.print(throttlePct);
  Serial.println(r == CANController::IOResult::OK ? " OK" : " FAIL");
}


void setup() 
{
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
  // put your main code here, to run repeatedly:

  uint8_t buf[8] = {0};

  uint16_t potValue = analogRead(potPin);
  potValue = constrain(potValue, ADC_MIN, ADC_MAX);
  throttlePct = map(potValue, ADC_MIN, ADC_MAX, 0, 100);

  /*  Logic: as throttle opens, pack current increases, and temperature increases. when throttle stays the same, temp stabilizes, pack current stabilizes. 
              closing throttle reduces pack current by same proportion, and temp also falls to normal. */

  
  float targetCurrentPct = (float)throttlePct;    //cast as float here cause we want throttlePct to stay as uint8_t elsewhere
  float targetTempPct =    (float)throttlePct;
  packCurrentPct += (targetCurrentPct - packCurrentPct)*CURRENT_ALPHA;
  tempPct += (targetTempPct - tempPct)*TEMP_ALPHA;

  if (packCurrentPct < 0) packCurrentPct = 0;
  if (packCurrentPct > 100) packCurrentPct = 100;
  if (tempPct < 0) tempPct = 0;
  if (tempPct > 100) tempPct = 100;
  
  build_494(buf, throttlePct);
  sendFrame(0x494, buf);

  build_6B0(buf, (uint32_t)packCurrentPct);
  sendFrame(0x6B0, buf);

  build_6B1(buf, (uint8_t)tempPct);
  sendFrame(0x6B1, buf);

}
