#include <Arduino.h>
#include <hdc1080_driver.hpp>
#include <usi_i2c_master.h>

#define HDC1080_I2C_ADDRESS 0x40

// Registers
#define HDC1080_TEMPERATURE_REGISTER 0x00
#define HDC1080_HUMIDITY_REGISTER 0x01
#define HDC1080_CONFIGURATION_REGISTER 0x02
#define HDC1080_MANUFACTURERID_REGISTER 0xFE
#define HDC1080_DEVICEID_REGISTER 0xFF
#define HDC1080_SERIALIDHIGH_REGISTER 0xFB
#define HDC1080_SERIALIDMID_REGISTER 0xFC
#define HDC1080_SERIALIDBOTTOM_REGISTER 0xFD

// Configuration Register Bits
#define HDC1080_CONFIG_RESET_BIT 0x8000
#define HDC1080_CONFIG_HEATER_ENABLE 0x2000
#define HDC1080_CONFIG_ACQUISITION_MODE 0x1000
#define HDC1080_CONFIG_BATTERY_STATUS 0x0800
#define HDC1080_CONFIG_TEMPERATURE_RESOLUTION 0x0400
#define HDC1080_CONFIG_HUMIDITY_RESOLUTION_HBIT 0x0200
#define HDC1080_CONFIG_HUMIDITY_RESOLUTION_LBIT 0x0100

#define HDC1080_CONFIG_TEMPERATURE_RESOLUTION_14BIT 0x0000
#define HDC1080_CONFIG_TEMPERATURE_RESOLUTION_11BIT 0x0400

#define HDC1080_CONFIG_HUMIDITY_RESOLUTION_14BIT 0x0000
#define HDC1080_CONFIG_HUMIDITY_RESOLUTION_11BIT 0x0100
#define HDC1080_CONFIG_HUMIDITY_RESOLUTION_8BIT 0x0200

HDC1080I2CDriver::HDC1080I2CDriver(bool use_14bit_conversion) {
  i2c_buffer[0] = HDC1080_CONFIGURATION_REGISTER;
  i2c_buffer[1] = 0x10;
  i2c_buffer[2] = 0x00;
  i2c_buffer[3] = 0x00;
}

void HDC1080I2CDriver::init() { i2c_send(HDC1080_I2C_ADDRESS, i2c_buffer, 3); }

ClimateData HDC1080I2CDriver::measure() {
  // trigger a measurement
  i2c_buffer[0] = 0x00;
  i2c_send(HDC1080_I2C_ADDRESS, i2c_buffer, 1);

  // datasheet: 14 bit measurements take about 6ms + 6ms -> 20ms
  delay(20);

  // receive the temperature and humidity:
  // 1 address byte + 2 byte temperature + 2 byte humidity
  i2c_receive(HDC1080_I2C_ADDRESS, i2c_buffer, 4);

  uint16_t temperature = (i2c_buffer[0] << 8) + i2c_buffer[1];
  uint16_t humidity = (i2c_buffer[2] << 8) + i2c_buffer[3];

  return ClimateData{((float)temperature / 65536) * 165 - 40,
                     ((float)humidity / 65536) * 100};
}