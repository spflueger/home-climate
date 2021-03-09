
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "hdc1080_driver.hpp"
#include <RH_ASK.h>

// for debugging purposes
#define USE_STACK_COUNTING 1

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 10  // not used, set to a non-existens pin
#define RH_TX_PIN 1   // Transmit pin
#define RH_PTT_PIN 10 // not used, set to a non-existens pin

// min max values for the ADC to calculate the battery percent
#define ADC_MIN 600 // ~1.5V (3*1V/2)
#define ADC_MAX 840 // ~2.1V (3*1.4V/2)

// time until the watchdog wakes the mc in seconds
#define WATCHDOG_TIME 2 // 1, 2, 4 or 8

// after how many watchdog wakeups we should collect and send the data
#define WATCHDOG_WAKEUPS_TARGET                                                \
  1 // 8 * 7 = 56 seconds between each data collection

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);
HDC1080I2CDriver hdc1080;

#if USE_STACK_COUNTING
extern uint8_t _end;
extern uint8_t __stack;

#define STACK_CANARY 0xC5

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
                 "    breq .loop");
#endif
}

uint16_t availableStackSize() {
  const uint8_t *p = &_end;
  uint16_t c = 0;

  while (*p == STACK_CANARY && p <= &__stack) {
    p++;
    c++;
  }

  return c;
}
#endif

void setupADC() {
  ADMUX = (1 << REFS2) |  // Sets ref. voltage to 2.56V internal
          (1 << REFS1) |  // Sets ref. voltage to 2.56V internal
          (0 << REFS0) |  // Sets ref. voltage to 2.56V internal
          (0 << MUX3) |   // use ADC2 for input (PB4), MUX bit 3
          (0 << MUX2) |   // use ADC2 for input (PB4), MUX bit 2
          (1 << MUX1) |   // use ADC2 for input (PB4), MUX bit 1
          (0 << MUX0);    // use ADC2 for input (PB4), MUX bit 0
  ADCSRA = (1 << ADEN) |  // enable ADC
           (1 << ADPS2) | // set prescaler to 64, bit 2
           (1 << ADPS1) | // set prescaler to 64, bit 1
           (0 << ADPS0);  // set prescaler to 64, bit 0

  // disable ADC for powersaving
  ADCSRA &= ~(1 << ADEN);

  // disable analog comperator for powersaving
  ACSR |= (1 << ACD);
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

uint8_t batteryLevel() {
  // In order to have a low power battery measurement, a pin of the attiny (here
  // pin3/PB3) is used to output the battery voltage and serves as a on off
  // switch of the current through the voltage divider. turn on PB3
  digitalWrite(PB3, HIGH);

  // enable the ADC
  ADCSRA |= (1 << ADEN);

  delay(10);

  ADCSRA |= (1 << ADSC); // start ADC measurement
  while (ADCSRA & (1 << ADSC))
    ; // wait till conversion complete

  int16_t adc = ADCL & (ADCH << 8);

  // clear the ADIF bit by writing 1 to it
  ADCSRA |= (1 << ADIF);

  // disable the ADC (power saving during power off state)
  ADCSRA &= ~(1 << ADEN);
  // turn off pin3
  digitalWrite(PB3, LOW);

  uint8_t battery_percentage = 100 * (adc - ADC_MIN) / (ADC_MAX - ADC_MIN);
  if (adc > ADC_MAX) {
    battery_percentage = 100;
  } else if (adc < ADC_MIN) {
    battery_percentage = 0;
  }
  return battery_percentage;
}

void setup() {
  hdc1080.init();

  if (!rh_driver.init()) {
    // do something in case init failed
  }

  // use PB3 pin as a voltage source for the battery measurement
  pinMode(PB3, OUTPUT);
  setupADC();
  enableWatchdog();
}

struct DataPackage {
  ClimateData climate_data;
  uint8_t battery_level;
  uint8_t station_id;
#if USE_STACK_COUNTING
  uint8_t available_stack_size;
#endif
};

void loop() {
  DataPackage data;
  data.climate_data = hdc1080.measure();
  data.battery_level = batteryLevel();
  data.station_id = 33;
#if USE_STACK_COUNTING
  data.available_stack_size = (uint8_t)availableStackSize();
#endif

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
