
#include <Arduino.h>
#include <usi_i2c_master.h>

#include <RH_ASK.h>

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 10  // not used, set to a non-existens pin
#define RH_TX_PIN 1   // Transmit pin
#define RH_PTT_PIN 10 // not used, set to a non-existens pin

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);

void initialize_hdc1080_sensor() {
  // first byte is hdc1080 address + LSB = 0 (LSB is read or write direction)
  uint8_t init_msg[3];
  init_msg[0] = 0x02;
  init_msg[1] = 0x10;
  init_msg[2] = 0x00;
  i2c_send(0x40, init_msg, 3);
}

struct ClimateData {
  float temperature;
  float humidity;
};

ClimateData data;

ClimateData make_measurement() {
  // trigger a measurement
  const uint8_t trigger_msg(0x00);
  i2c_send(0x40, &trigger_msg, 1);

  // datasheet: 14 bit measurements take about 6ms + 6ms -> 20ms
  delay(20);

  // receive the temperature and humidity:
  // 1 address byte + 2 byte temperature + 2 byte humidity
  uint8_t msg[4];
  i2c_receive(0x40, msg, 4);

  uint16_t temp = (msg[0] << 8) + msg[1];
  uint16_t hum = (msg[2] << 8) + msg[3];

  data.temperature = ((float)temp / 65536) * 165 - 40;
  data.humidity = ((float)hum / 65536) * 100;
}

void setup() {
  initialize_hdc1080_sensor();

  // init RadioHead
  if (!rh_driver.init()) {
    // do something in case init failed
  }
}

void loop() {
  ClimateData data(make_measurement());

  rh_driver.send((uint8_t *)&data, sizeof(data));

  rh_driver.waitPacketSent();

  delay(1000);
}