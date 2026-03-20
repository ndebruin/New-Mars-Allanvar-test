#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include "LSM6DSO32Sensor.h"
#include "ASM330LHHSensor.h"
#include "LIS2MDLSensor.h"
#include "LPS22HBSensor.h"
#include "../LoRaE22/LoRaE22.h"
#include "../include/RadioConfigs.h"

// initialize our sensor buses 
SPIClass SENSORS_SPI(SENSORS_SPI_MOSI, SENSORS_SPI_MISO, SENSORS_SPI_SCK);
TwoWire GPS_I2C(GPS_I2C_SDA, GPS_I2C_SCL);
HardwareSerial GPS_SERIAL(GPS_SERIAL_RX, GPS_SERIAL_TX);
TwoWire CONNECTOR_I2C(CONNECTOR_I2C_SDA, CONNECTOR_I2C_SCL);
SPIClass CAMERA_SPI(CAMERA_MOSI, CAMERA_MISO, CAMERA_SCK);
HardwareSerial RADIO_SERIAL(RADIO_SERIAL_RX, RADIO_SERIAL_TX);

LSM6DSO32Sensor lsm(&SENSORS_SPI, SENSORS_LSM_CS);
ASM330LHHSensor asmSens(&SENSORS_SPI, SENSORS_ASM_CS);
LIS2MDLSensor lis(&SENSORS_SPI, SENSORS_LIS_CS);
LPS22HBSensor lps(&SENSORS_SPI, SENSORS_LPS_CS);

// #define SENSOR_DEBUG

float g = 9.81;

static constexpr uint32_t HZ = 40;
static constexpr uint32_t DT = 1000000UL / HZ;
static constexpr uint32_t TOTAL_TIME = 7UL * 3600UL;
static constexpr uint32_t TOTAL_SAMPLES = HZ * TOTAL_TIME;

struct MAX10SData {
    float lat;
    float lon;
    float ecefX;
    float ecefY;
    float ecefZ;
    float altMSL;
    float altEllipsoid;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    uint32_t epochTime;
    uint8_t satellites;
    uint8_t gpsLockType;
};

//Sample struct
struct Sample {
  float lsm_ax, lsm_ay, lsm_az;
  float lsm_gx, lsm_gy, lsm_gz;
  float lis_mx, lis_my, lis_mz;
  float asm_ax, asm_ay, asm_az;
  float asm_gx, asm_gy, asm_gz;
  float lps_p, lps_t;
};

static inline void zeroSample(Sample &s) {
  s.lsm_ax = s.lsm_ay = s.lsm_az = 0.0f;
  s.lsm_gx = s.lsm_gy = s.lsm_gz = 0.0f;
  s.lis_mx = s.lis_my = s.lis_mz = 0.0f;
  s.asm_ax = s.asm_ay = s.asm_az = 0.0f;
  s.asm_gx = s.asm_gy = s.asm_gz = 0.0f;
  s.lps_p = s.lps_t = 0.0f;
}

//Read sensor data
static bool readSensorData(Sample &s) {
    zeroSample(s);


    int32_t lsmAccel[3];
    int32_t asmAccel[3];
    int32_t mag[3];
    int32_t asmGyro[3];
    int32_t lsmGyro[3];
    float pressure, temperature;
    lsm.Get_X_Axes(lsmAccel);
    lsm.Get_G_Axes(lsmGyro);
    asmSens.Get_X_Axes(asmAccel);
    asmSens.Get_G_Axes(asmGyro);
    lis.GetAxes(mag);
    lps.GetPressure(&pressure);
    lps.GetTemperature(&temperature);

    s.lps_p = pressure * 100; // mbar -> pa
    s.lps_t = temperature;

    s.lsm_ax = lsmAccel[0] / 1000.0 * g;
    s.lsm_ay = lsmAccel[1] / 1000.0 * g;
    s.lsm_az = lsmAccel[2] / 1000.0 * g;

    s.lsm_gx = lsmGyro[0] / 1000.0 * (PI / 180.0f);
    s.lsm_gy = lsmGyro[1] / 1000.0 * (PI / 180.0f);
    s.lsm_gz = lsmGyro[2] / 1000.0 * (PI / 180.0f);

    s.lis_mx = mag[0];
    s.lis_my = mag[1];
    s.lis_mz = mag[2];

    s.asm_ax = asmAccel[0] / 1000.0 * g;
    s.asm_ay = asmAccel[1] / 1000.0 * g;
    s.asm_az = asmAccel[2] / 1000.0 * g;

    s.asm_gx = asmGyro[0] / 1000.0f * (PI / 180.0f);
    s.asm_gy = asmGyro[1] / 1000.0f * (PI / 180.0f);
    s.asm_gz = asmGyro[2] / 1000.0f * (PI / 180.0f);

  return true;
}

void sensorInit(){
    pinMode(SENSORS_ASM_CS, OUTPUT); digitalWrite(SENSORS_ASM_CS, HIGH);
    pinMode(SENSORS_LSM_CS, OUTPUT); digitalWrite(SENSORS_LSM_CS, HIGH);
    pinMode(SENSORS_LIS_CS, OUTPUT); digitalWrite(SENSORS_LIS_CS, HIGH);
    pinMode(SENSORS_LPS_CS, OUTPUT); digitalWrite(SENSORS_LPS_CS, HIGH);

    #ifdef SENSOR_DEBUG
        SerialUSB.println("spi begin");
    #endif

    SENSORS_SPI.begin();

    LSM6DSO32StatusTypeDef lsmBeginStatus = lsm.begin();
    #ifdef SENSOR_DEBUG
        SerialUSB.println(lsmBeginStatus);
        SerialUSB.println("lsm begin");
    #endif
    lsm.Set_G_FS(2000);
    lsm.Set_G_ODR(100);
    lsm.Set_X_FS(32);
    lsm.Set_X_ODR(500);
    lsm.Enable_X();
    lsm.Enable_G();

    delay(20);

    ASM330LHHStatusTypeDef asmBeginStatus = asmSens.begin();
    #ifdef SENSOR_DEBUG
        SerialUSB.println(asmBeginStatus);
        SerialUSB.println("asm begin");
    #endif    
    asmSens.Set_G_FS(4000);
    asmSens.Set_G_ODR(100);
    asmSens.Set_X_FS(16);
    asmSens.Set_X_ODR(500);
    asmSens.Enable_X();
    asmSens.Enable_G();
    
    delay(20);

    LIS2MDLStatusTypeDef lisBeginStatus = lis.begin();
    #ifdef SENSOR_DEBUG
        SerialUSB.println(lisBeginStatus);
        SerialUSB.println("lis begin");
    #endif
    lis.SetOutputDataRate(100);
    lis.Enable();
    
    delay(20);
    
    LPS22HBStatusTypeDef lpsBeginStatus = lps.begin();
    #ifdef SENSOR_DEBUG
        SerialUSB.println(lpsBeginStatus);
        SerialUSB.println("lps begin");
    #endif
    lps.SetODR(100);
    lps.Enable();
}

void setup()
{
    delay(2000);
    Serial.println("test");
    SerialUSB.begin(); while(!SerialUSB.available()){};
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);

    sensorInit();
    
    delay(10000);
    SerialUSB.println("READY");
    SerialUSB.println("index,t_us,lsm_ax,lsm_ay,lsm_az,lsm_gx,lsm_gy,lsm_gz,lis_mx,lis_my,lis_mz,asm_ax,asm_ay,asm_az,asm_gx,asm_gy,asm_gz,lps_p,lps_t");

}

void loop()
{    
  static uint32_t start_us = micros();
  static uint32_t next_us  = start_us;
  static uint32_t index = 0;
  static bool done = false;

  if (done) {
    delay(1000);
    return;
  }

  uint32_t now = micros();
  if ((int32_t)(now - next_us) < 0) return;
  next_us += DT;

  if (index >= TOTAL_SAMPLES) {
    done = true;
    return;
  }

  digitalToggle(LED_GREEN);

  Sample s;
  readSensorData(s);

  uint32_t t_us = now - start_us;

  SerialUSB.print(index); SerialUSB.print(',');
  SerialUSB.print(t_us); SerialUSB.print(',');
  SerialUSB.print(s.lsm_ax, 5); SerialUSB.print(',');
  SerialUSB.print(s.lsm_ay, 5); SerialUSB.print(',');
  SerialUSB.print(s.lsm_az, 5); SerialUSB.print(',');
  SerialUSB.print(s.lsm_gx, 5); SerialUSB.print(',');
  SerialUSB.print(s.lsm_gy, 5); SerialUSB.print(',');
  SerialUSB.print(s.lsm_gz, 5); SerialUSB.print(',');
  SerialUSB.print(s.lis_mx); SerialUSB.print(',');
  SerialUSB.print(s.lis_my); SerialUSB.print(',');
  SerialUSB.print(s.lis_mz); SerialUSB.print(',');
  SerialUSB.print(s.asm_ax, 5); SerialUSB.print(',');
  SerialUSB.print(s.asm_ay, 5); SerialUSB.print(',');
  SerialUSB.print(s.asm_az, 5); SerialUSB.print(',');
  SerialUSB.print(s.asm_gx, 5); SerialUSB.print(',');
  SerialUSB.print(s.asm_gy, 5); SerialUSB.print(',');
  SerialUSB.print(s.asm_gz, 5); SerialUSB.print(',');
  SerialUSB.print(s.lps_p); SerialUSB.print(',');
  SerialUSB.println(s.lps_t);

  index++;
}