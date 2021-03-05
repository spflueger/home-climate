#include <Arduino.h>

struct ClimateData {
  float temperature;
  float humidity;
};

class HDC1080I2CDriver {
public:
  HDC1080I2CDriver(bool use_14bit_conversion = true);

  void init();
  ClimateData measure();

private:
  uint8_t i2c_buffer[4];
};