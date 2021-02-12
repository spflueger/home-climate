/*-----------------------------------------------------*\
|  USI I2C Slave Master                                 |
|                                                       |
| This library provides a robust I2C master protocol    |
| implementation on top of Atmel's Universal Serial     |
| Interface (USI) found in many ATTiny microcontrollers.|
|                                                       |
| Adam Honse (GitHub: CalcProgrammer1) - 7/29/2012      |
|            -calcprogrammer1@gmail.com                 |
\*-----------------------------------------------------*/

#include "usi_i2c_master.h"
#include <avr/interrupt.h>

///////////////////////////////////////////////////////////////////////////////
////USI Master Macros//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define USISR_TRANSFER_8_BIT 0b11110000 | (0x00 << USICNT0)
#define USISR_TRANSFER_1_BIT 0b11110000 | (0x0E << USICNT0)

#define USICR_CLOCK_STROBE_MASK 0b00101011

#define USI_CLOCK_STROBE()                                                     \
  { USICR = USICR_CLOCK_STROBE_MASK; }

#define USI_SET_SDA_OUTPUT()                                                   \
  { DDR_USI |= (1 << PORT_USI_SDA); }
#define USI_SET_SDA_INPUT()                                                    \
  { DDR_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SDA_HIGH()                                                     \
  { PORT_USI |= (1 << PORT_USI_SDA); }
#define USI_SET_SDA_LOW()                                                      \
  { PORT_USI &= ~(1 << PORT_USI_SDA); }

#define USI_SET_SCL_OUTPUT()                                                   \
  { DDR_USI |= (1 << PORT_USI_SCL); }
#define USI_SET_SCL_INPUT()                                                    \
  { DDR_USI &= ~(1 << PORT_USI_SCL); }

#define USI_SET_SCL_HIGH()                                                     \
  { PORT_USI |= (1 << PORT_USI_SCL); }
#define USI_SET_SCL_LOW()                                                      \
  { PORT_USI &= ~(1 << PORT_USI_SCL); }

#define USI_I2C_WAIT_HIGH()                                                    \
  { _delay_us(I2C_THIGH); }
#define USI_I2C_WAIT_LOW()                                                     \
  { _delay_us(I2C_TLOW); }

/////////////////////////////////////////////////////////////////////
// USI_I2C_Master_Transfer                                         //
//  Transfers either 8 bits (data) or 1 bit (ACK/NACK) on the bus. //
/////////////////////////////////////////////////////////////////////

uint8_t USI_I2C_Master_Transfer(uint8_t USISR_temp) {
  USISR = USISR_temp; // Set USISR as requested by calling function

  // Shift Data
  do {
    USI_I2C_WAIT_LOW();
    USI_CLOCK_STROBE(); // SCL Positive Edge
    while (!(PIN_USI & (1 << PIN_USI_SCL)))
      ; // Wait for SCL to go high
    USI_I2C_WAIT_HIGH();
    USI_CLOCK_STROBE();               // SCL Negative Edge
  } while (!(USISR & (1 << USIOIF))); // Do until transfer is complete

  USI_I2C_WAIT_LOW();

  return USIDR;
}

void create_start_condition() {
  USI_SET_SCL_HIGH(); // Setting input makes line pull high

  while (!(PIN_USI & (1 << PIN_USI_SCL)))
    ; // Wait for SCL to go high

#ifdef I2C_FAST_MODE
  USI_I2C_WAIT_HIGH();
#else
  USI_I2C_WAIT_LOW();
#endif
  USI_SET_SDA_OUTPUT();
  USI_SET_SCL_OUTPUT();
  USI_SET_SDA_LOW();
  USI_I2C_WAIT_HIGH();
  USI_SET_SCL_LOW();
  USI_I2C_WAIT_LOW();
  USI_SET_SDA_HIGH();
}

void send_stop_condition() {

  USI_SET_SDA_LOW(); // Pull SDA low.
  USI_I2C_WAIT_LOW();

  USI_SET_SCL_INPUT(); // Release SCL.

  while (!(PIN_USI & (1 << PIN_USI_SCL)))
    ; // Wait for SCL to go high.

  USI_I2C_WAIT_HIGH();
  USI_SET_SDA_INPUT(); // Release SDA.

  while (!(PIN_USI & (1 << PIN_USI_SDA)))
    ; // Wait for SDA to go high.
}

uint8_t send_byte(uint8_t data) {
  ///////////////////////////////////////////////////////////////////
  // Write Operation                                               //
  //  Writes a byte to the slave and checks for ACK                //
  //  If no ACK, then reset and exit                               //
  ///////////////////////////////////////////////////////////////////

  USI_SET_SCL_LOW();

  USIDR = data; // Load data

  USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);

  USI_SET_SDA_INPUT();

  if (USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT) & 0x01) {
    USI_SET_SCL_HIGH();
    USI_SET_SDA_HIGH();
    return 0;
  }

  USI_SET_SDA_OUTPUT();
  return 1;
}

uint8_t i2c_send(const uint8_t address, const uint8_t *message_buffer,
                 uint8_t bytes_to_send) {
  create_start_condition();

  if (!send_byte((address << 1) | 0x00)) {
    return 0;
  }

  do {
    if (!send_byte(*message_buffer)) {
      return 0;
    }
    ++message_buffer;
  } while (--bytes_to_send);

  send_stop_condition();
  return 1;
}

void i2c_receive(const uint8_t address, uint8_t *message_buffer,
                 uint8_t bytes_to_receive) {
  uint8_t *p_msg = &message_buffer[0];
  create_start_condition();

  send_byte((address << 1) | 0x01);

  do {
    ///////////////////////////////////////////////////////////////////
    // Read Operation                                                //
    //  Reads a byte from the slave and sends ACK or NACK            //
    ///////////////////////////////////////////////////////////////////
    USI_SET_SDA_INPUT();

    *p_msg = USI_I2C_Master_Transfer(USISR_TRANSFER_8_BIT);

    USI_SET_SDA_OUTPUT();

    if (bytes_to_receive == 1) {
      USIDR = 0xFF; // Load NACK to end transmission
    } else {
      USIDR = 0x00; // Load ACK
    }

    USI_I2C_Master_Transfer(USISR_TRANSFER_1_BIT);
    ++p_msg;
  } while (--bytes_to_receive); // Do until all data is read/written

  send_stop_condition();
  return;
}