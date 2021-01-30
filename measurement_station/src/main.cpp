#include <Arduino.h>

#include <RH_ASK.h>

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 5 // not used, set to a non-existens pin
#define RH_TX_PIN 3
#define RH_PTT_PIN 5 // not used, set to a non-existens pin

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);

void setup() {
  // init RadioHead
  if(!rh_driver.init()){
    // do something in case init failed
  }
}

// main loop
void loop() {
  const char *msg = "hello world";

  rh_driver.send((uint8_t *)msg, strlen(msg));
  rh_driver.waitPacketSent();

  delay(2000);
}