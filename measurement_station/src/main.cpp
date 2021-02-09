
#include <Arduino.h>
#include <usi_i2c_master.h>


#include <RH_ASK.h>

// RadioHead bitrate in bit/s
#define RH_SPEED 2000

// pins for the radio hardware
#define RH_RX_PIN 10 // not used, set to a non-existens pin
#define RH_TX_PIN  1 // Transmit pin
#define RH_PTT_PIN 10 // not used, set to a non-existens pin

RH_ASK rh_driver(RH_SPEED, RH_RX_PIN, RH_TX_PIN, RH_PTT_PIN);



String sC;
char msg[6] = "\x80\x02\x10\x00";

void setup() {
  //I2C
  // unsigned char* msg[] = {0x81, 0x02, 0x10, 0x00}; // write the config register adr 2 with bit 12 on
 
//  sC = "\x80\x02\x10\x00";

  USI_I2C_Master_Start_Transmission( msg, '\x04');


  // init RadioHead
  if(!rh_driver.init()){
    // do something in case init failed
  }
}

// main loop
void loop() {
// char msg[]= "\x80\x00\x00\x00";
msg[0]= '\x80'; msg[1]= '\x00'; msg[2] = '\x00'; msg[3]= '\x00';
USI_I2C_Master_Start_Transmission(  msg, '\x2');
// sC = "\x80\x01\x00\x00"; // write Temp register to start measurement

// msg[1]= '\x01'; 
// USI_I2C_Master_Start_Transmission(  msg, '\x2');


delay(1000);

struct ClimateData {
      float h;
      float t;
    };

ClimateData data = ClimateData();

msg[0]= '\x81';   // read the temp and humidity
USI_I2C_Master_Start_Transmission(  msg, '\x5');


uint16_t temp;
temp = (msg[1]<<8) + msg[2];
data.t = ((float)temp / 65536) * 165 - 40;

uint16_t hum =  (msg[3] << 8) + msg[4];
data.h= ((float)hum / 65536) * 100;


    
rh_driver.send((uint8_t*)&data, sizeof(data));

  // rh_driver.send((uint8_t *)msg, strlen(msg));
rh_driver.waitPacketSent();

  // delay(1000);
}