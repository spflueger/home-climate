
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "hdc1080_driver.hpp"
#include <RH_ASK.h>

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 10  // not used, set to a non-existens pin
#define RH_TX_PIN 1   // Transmit pin
#define RH_PTT_PIN 10 // not used, set to a non-existens pin

// time until the watchdog wakes the mc in seconds
#define WATCHDOG_TIME 2 // 1, 2, 4 or 8

// after how many watchdog wakeups we should collect and send the data
#define WATCHDOG_WAKEUPS_TARGET                                                \
  1 // 8 * 7 = 56 seconds between each data collection

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

void enableWatchdog() {
  cli();

  // clear the reset flag
  MCUSR &= ~(1 << WDRF);

  // set WDCE to be able to change/set WDE
  WDTCR |= (1 << WDCE) | (1 << WDE);

// set new watchdog timeout prescaler value
#if WATCHDOG_TIME == 1
  WDTCR = 1 << WDP1 | 1 << WDP2;
#elif WATCHDOG_TIME == 2
  WDTCR = 1 << WDP0 | 1 << WDP1 | 1 << WDP2;
#elif WATCHDOG_TIME == 4
  WDTCR = 1 << WDP3;
#elif WATCHDOG_TIME == 8
  WDTCR = 1 << WDP0 | 1 << WDP3;
#else
#error WATCHDOG_TIME must be 1, 2, 4 or 8!
#endif

  // enable the WD interrupt to get an interrupt instead of a reset
  WDTCR |= (1 << WDIE);

  sei();
}

void enterSleep(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Now enter sleep mode
  sleep_mode();

  // The program will continue from here after the WDT timeout
  // First thing to do is disable sleep
  sleep_disable();
}

void setup() {
  hdc1080.init();

  if (!rh_driver.init()) {
    // do something in case init failed
  }

  enableWatchdog();
}

void loop() {
  ClimateData data = hdc1080.measure();

  rh_driver.send((uint8_t *)&data, sizeof(data));
  rh_driver.waitPacketSent();

  // deep sleep
  for (uint8_t i = 0; i < WATCHDOG_WAKEUPS_TARGET; i++) {
    enterSleep();
  }
}

// watchdog ISR
ISR(WDT_vect) {
  // nothing to do here, just wake up
}