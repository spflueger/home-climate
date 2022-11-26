
#include <Arduino.h>

// #include "hdc1080_driver.hpp"

#include <RH_ASK.h>
#define USE_STACK_COUNTING 0
// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN PD3 // receive pin
#define RH_TX_PIN 33  // not used, set to a non-existent pin
#define RH_PTT_PIN 33 // not used, set to a non-existent pin

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);

void setup() {
  Serial.begin(9600);
  if (!rh_driver.init()) {
    // do something in case init failed
  }
}

struct ClimateData {
  float temperature;
  float humidity;
};

struct DataPackage {
  ClimateData climate_data;
  uint8_t battery_level;
  uint8_t station_id;
#if USE_STACK_COUNTING
  uint8_t available_stack_size;
#endif
};

DataPackage data;
uint8_t data_length = sizeof(data);

void loop() {
    if (rh_driver.recv((uint8_t*)&data, &data_length)) // Non-blocking
    {
        Serial.println("received dataframe:");
        Serial.println(data.climate_data.temperature);
        Serial.println(data.climate_data.humidity);
        Serial.println(data.battery_level);
        Serial.println(data.station_id);
    }
}