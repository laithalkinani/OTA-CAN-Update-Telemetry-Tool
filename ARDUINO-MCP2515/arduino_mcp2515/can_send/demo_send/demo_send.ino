#include "AA_MCP2515.h"

/*Constants*/
#define ADC_MIN ((uint16_t)0)
#define ADC_MAX ((uint16_t)1023)
#define MAX_ERPM ((uint32_t)50000)
#define MIN_MOTOR_TEMP ((float)20.0)
#define MAX_MOTOR_TEMP ((float)90.0)
#define CURRENT_ALPHA ((float)0.15)   //arbitrary value for current delta inertia
#define TEMP_ALPHA    ((float)0.02)   //arbitrary value for temp delta inertia
#define SOC_ALPHA     ((float)0.1)    //arbitrary value for soc decrease coefficient
#define SOC_START     ((float)80.0)
#define SOC_MIN       ((float)10.0)
#define SOC_MAX       ((float)80.0)      //to model soc loss
#define V_MAX         ((float)400.0)
#define V_MIN         ((float)350.0)
#define ACCEL_GAIN    ((float)50000.0)   // ERPM/s at 100% throttle before drag
#define DRAG_COEFF    ((float)1.0)       //arbitrary drag coefficient honestly


const CANBitrate::Config CAN_BITRATE = CANBitrate::Config_8MHz_500kbps;
const uint8_t  CAN_PIN_CS  = 10;
const int8_t   CAN_PIN_INT = 2;
const uint32_t CAN_SPI_HZ  = 2000000;
const uint8_t potPin = A0;
CANConfig config(CAN_BITRATE, CAN_PIN_CS, CAN_PIN_INT, SPI, CAN_SPI_HZ);
CANController CAN(config);

/*Initializations*/
uint8_t throttlePct = 0;
float packCurrentPct = 0.0;
float tempPct = 0.0;
float packSOC = SOC_START;
float lastMillis = 0;
float currentRpm = 0;
float accel = 0;
float dutyCycle = 0;
float packVoltage = 0;


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

void build_6B0(uint8_t* buf, uint32_t input, uint32_t socInput) 
{
  memset(buf, 0, 8);
  packSignalBE(buf,  7, 16, input * 10);        // Pack_Current: 0–100 A
  packSignalBE(buf, 23, 16, 3000 + input * 10); // Pack_Inst_Voltage: 300–400 V
  packSignalBE(buf, 39,  8, socInput * 2);         // Pack_SOC: 0–100%
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

void build_414(uint8_t* buf, float rpmInput, float dutyInput, float voltageInput)
{
  memset(buf, 0, 8);
  packSignalBE(buf,  7, 32, (uint32_t)rpmInput);          // Actual_ERPM: factor 1
  packSignalBE(buf, 39, 16, (uint32_t)(dutyInput * 10));   // Actual_Duty: factor 0.1
  packSignalBE(buf, 55, 16, (uint32_t)voltageInput);       // Actual_InputVoltage: factor 1
}

/*  CAN Send Frame Function */

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

float returnAccel(uint8_t throttle, float currentRpm)
{
  float accel = ACCEL_GAIN * (throttle / 100.0);   // torque demand from throttle, 0 at throttle = 0, 50000 at throttle = 100
  float drag  = DRAG_COEFF * currentRpm;           // resistance (drag) proportional to speed
  float netAccel = accel - drag;

  return netAccel;
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

  //warning: buf gets accessed by multiple function calls inside loop
  uint8_t buf[8] = {0};   //zero it again even tho it's zeroed out each function call, why not
  float now = millis();
  float dt = (now - lastMillis)/1000.0;
  lastMillis = now;   //next timestep
  

  uint16_t potValue = analogRead(potPin);
  potValue = constrain(potValue, ADC_MIN, ADC_MAX);
  throttlePct = map(potValue, ADC_MIN, ADC_MAX, 0, 100);

  /*  Logic: as throttle opens, pack current increases, and temperature increases. when throttle stays the same, temp stabilizes, pack current stabilizes. 
      Closing throttle reduces pack current by same proportion, and temp also falls to normal.
      RPM is the integral of acceleration, and slowly tracks throttle position.
      Duty Cycle is 100 when currentRpm = MAX_ERPM, 0 when currentRpm = 0.
      Pack Voltage tracks SOC as a linear interpolation: lerp(a,b,t) = a + (b-a)*t*/

  
  float targetCurrentPct = (float)throttlePct;    //cast as float here cause we want throttlePct to stay as uint8_t elsewhere
  float targetTempPct =    (float)throttlePct;
  packCurrentPct += (targetCurrentPct - packCurrentPct)*CURRENT_ALPHA;
  tempPct += (targetTempPct - tempPct)*TEMP_ALPHA;
  packSOC -= (throttlePct / 100.0)*SOC_ALPHA;

  accel = returnAccel(throttlePct, currentRpm);
  currentRpm += accel * dt;

  dutyCycle = (currentRpm / MAX_ERPM) * 100.0;
  packVoltage = V_MIN + (V_MAX - V_MIN) * ((packSOC - SOC_MIN) / (SOC_MAX - SOC_MIN));    //i am lerping

  /*Clamps*/
  if (packCurrentPct < 0) packCurrentPct = 0;
  if (packCurrentPct > 100) packCurrentPct = 100;
  if (tempPct < 0) tempPct = 0;
  if (tempPct > 100) tempPct = 100;
  if (packSOC <= SOC_MIN) packSOC = SOC_MAX;
  if (currentRpm < 0.0) currentRpm = 0.0;
  if (currentRpm > MAX_ERPM) currentRpm = MAX_ERPM;
  if (dutyCycle < 0.0) dutyCycle = 0.0;
  if (dutyCycle > 100.0) dutyCycle = 100.0;
  if (packVoltage < V_MIN) packVoltage = V_MIN;
  if (packVoltage > V_MAX) packVoltage = V_MAX;
  


  /*Build each frame, then send it*/
  //each function call zeroes out buf so garbage values aren't accidentally being sent in the next frame
  build_414(buf, currentRpm, dutyCycle, packVoltage);
  sendFrame(0x414, buf);

  build_494(buf, throttlePct);
  sendFrame(0x494, buf);

  build_6B0(buf, (uint32_t)packCurrentPct, (uint32_t)packSOC);
  sendFrame(0x6B0, buf);

  build_6B1(buf, (uint8_t)tempPct);
  sendFrame(0x6B1, buf);

}
