
#include <Arduino.h>

#include "hdc1080_driver.hpp"
#include <RH_ASK.h>

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 10  // not used, set to a non-existens pin
#define RH_TX_PIN 1   // Transmit pin
#define RH_PTT_PIN 10 // not used, set to a non-existens pin

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);
HDC1080I2CDriver hdc1080;

extern uint8_t _end;
extern uint8_t __stack;

#define STACK_CANARY 0xc5;

void StackPaint(void) __attribute__((naked)) __attribute__((optimize("O0")))
__attribute__((section(".init1")));

void StackPaint(void) {
#if 0
  uint8_t *p = &_end;

  while (p <= &__stack) {
    *p = STACK_CANARY;
    p++;
  }
#else
  __asm volatile("    ldi r30,lo8(_end)\n"
                 "    ldi r31,hi8(_end)\n"
                 "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                 "    ldi r25,hi8(__stack)\n"
                 "    rjmp .cmp\n"
                 ".loop:\n"
                 "    st Z+,r24\n"
                 ".cmp:\n"
                 "    cpi r30,lo8(__stack)\n"
                 "    cpc r31,r25\n"
                 "    brlo .loop\n"
                 "    breq .loop" ::);
#endif
}

void setup() {
  hdc1080.init();

  if (!rh_driver.init()) {
    // do something in case init failed
  }
}

void loop() {
  ClimateData data = hdc1080.measure();

  rh_driver.send((uint8_t *)&data, sizeof(data));
  rh_driver.waitPacketSent();
  delay(1000);
}