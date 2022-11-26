# Originally code from this source:
#
# SDL_Pi_HDC1080
# Python 3
# Raspberry Pi Driver for the SwitchDoc Labs HDC1080 Breakout Board
#
# SwitchDoc Labs
# December 2018
#
# Version 1.1

import array
import time
import io
import fcntl
from typing import Optional, Tuple, Callable

# Constants
# I2C Address
HDC1080_ADDRESS = 0x40  # 1000000
# Registers
HDC1080_TEMPERATURE_REGISTER = 0x00
HDC1080_HUMIDITY_REGISTER = 0x01
HDC1080_CONFIGURATION_REGISTER = 0x02
HDC1080_MANUFACTURERID_REGISTER = 0xFE
HDC1080_DEVICEID_REGISTER = 0xFF
HDC1080_SERIALIDHIGH_REGISTER = 0xFB
HDC1080_SERIALIDMID_REGISTER = 0xFC
HDC1080_SERIALIDBOTTOM_REGISTER = 0xFD

# Configuration Register Bits
HDC1080_CONFIG_RESET_BIT = 0x8000
HDC1080_CONFIG_HEATER_ENABLE = 0x2000
HDC1080_CONFIG_ACQUISITION_MODE = 0x1000
HDC1080_CONFIG_BATTERY_STATUS = 0x0800
HDC1080_CONFIG_DUAL_READ_MODE_ENABLE = 0x0010

HDC1080_CONFIG_TEMPERATURE_RESOLUTION = 0x0400
HDC1080_CONFIG_HUMIDITY_RESOLUTION_HBIT = 0x0200
HDC1080_CONFIG_HUMIDITY_RESOLUTION_LBIT = 0x0100

HDC1080_CONFIG_TEMPERATURE_RESOLUTION_14BIT = 0x0000
HDC1080_CONFIG_TEMPERATURE_RESOLUTION_11BIT = 0x0400

HDC1080_CONFIG_HUMIDITY_RESOLUTION_14BIT = 0x0000
HDC1080_CONFIG_HUMIDITY_RESOLUTION_11BIT = 0x0100
HDC1080_CONFIG_HUMIDITY_RESOLUTION_8BIT = 0x0200

I2C_SLAVE = 0x0703


class SDL_Pi_HDC1080:
    def __init__(
        self,
        dual_read_mode: bool = True,
        heater_on: bool = False,
        twi: int = 1,
        addr: int = HDC1080_ADDRESS,
        temperature_converter: Optional[Callable[[int], float]] = None,
        humidity_converter: Optional[Callable[[int], float]] = None,
    ):
        self.__HDC1080_fw = io.open("/dev/i2c-" + str(twi), "wb", buffering=0)
        self.__HDC1080_fr = io.open("/dev/i2c-" + str(twi), "rb", buffering=0)
        self.__temp_converter = (
            lambda x: (x / 65536.0) * 165.0 - 40
            if temperature_converter is None
            else temperature_converter
        )
        self.__humi_converter = (
            lambda x: x / 65536.0 * 100.0
            if humidity_converter is None
            else humidity_converter
        )

        # set device address
        fcntl.ioctl(self.__HDC1080_fr, I2C_SLAVE, addr)
        fcntl.ioctl(self.__HDC1080_fw, I2C_SLAVE, addr)
        time.sleep(0.015)  # 15ms startup time

        config = HDC1080_CONFIG_ACQUISITION_MODE
        if dual_read_mode:
            config = config | HDC1080_CONFIG_DUAL_READ_MODE_ENABLE
        if heater_on:
            config = config | HDC1080_CONFIG_HEATER_ENABLE

        self.__HDC1080_fw.write(
            bytearray(
                [
                    HDC1080_CONFIGURATION_REGISTER,
                    config >> 8,
                    0x00,
                ]
            )
        )  # sending config register bytes
        time.sleep(0.015)  # From the data sheet

    def read_temperature(self) -> float:
        time.sleep(0.015)  # From the data sheet
        self.__HDC1080_fw.write(bytearray([HDC1080_TEMPERATURE_REGISTER]))
        time.sleep(0.01)  # From the data sheet

        data = self.__HDC1080_fr.read(2)  # read 2 byte temperature data
        buf = array.array("B", data)

        # Convert the data
        raw_temp = (buf[0] * 256) + buf[1]
        return self.__temp_converter(raw_temp)

    def read_humidity(self):
        self.__HDC1080_fw.write(bytearray([HDC1080_HUMIDITY_REGISTER]))
        time.sleep(0.01)  # From the data sheet

        data = self.__HDC1080_fr.read(2)  # read 2 byte humidity data
        buf = array.array("B", data)
        raw_humidity = (buf[0] * 256) + buf[1]
        return self.__humi_converter(raw_humidity)

    def read_temperature_and_humidity(self) -> Tuple[float, float]:
        # Trigger the measurements by executing a pointer write transaction
        # with the address pointer set to 0x00. Refer to Figure 12.
        # self.__bus.write_byte(self.__device_address, 0x00)
        self.__HDC1080_fw.write(bytearray([HDC1080_TEMPERATURE_REGISTER]))
        # Wait for the measurements to complete, based on the conversion time
        time.sleep(0.02)
        # Read the temperature data from register address 0x00, followed by
        # the humidity data from register address 0x01 in a single transaction
        # as shown in Figure 14. A read operation will return a NACK if the
        # contents of the registers have not been updated as shown in
        data = self.__HDC1080_fr.read(4)
        buf = array.array("B", data)
        temp_raw = (buf[0] * 256) + buf[1]
        humi_raw = (buf[2] * 256) + buf[3]

        return (
            self.__temp_converter(temp_raw),
            self.__humi_converter(humi_raw),
        )

    def read_config_register(self):
        # Read config register
        self.__HDC1080_fw.write(bytearray([HDC1080_CONFIGURATION_REGISTER]))
        time.sleep(0.01)  # From the data sheet

        data = self.__HDC1080_fr.read(2)  # read 2 byte config data

        buf = array.array("B", data)

        return buf[0] * 256 + buf[1]

    def turn_heater_on(self):
        # Read config register
        config = self.read_config_register()
        config = config | HDC1080_CONFIG_HEATER_ENABLE

        self.__HDC1080_fw.write(
            bytearray([HDC1080_CONFIGURATION_REGISTER, config >> 8, 0x00])
        )  # sending config register bytes
        time.sleep(0.015)  # From the data sheet

    def turn_heater_off(self):
        # Read config register
        config = self.read_config_register()
        config = config & ~HDC1080_CONFIG_HEATER_ENABLE

        self.__HDC1080_fw.write(
            bytearray([HDC1080_CONFIGURATION_REGISTER, config >> 8, 0x00])
        )  # sending config register bytes
        time.sleep(0.015)  # From the data sheet

    def set_humidity_resolution(self, resolution):
        # Read config register
        config = self.read_config_register()
        config = (config & ~0x0300) | resolution

        self.__HDC1080_fw.write(
            bytearray([HDC1080_CONFIGURATION_REGISTER, config >> 8, 0x00])
        )  # sending config register bytes
        time.sleep(0.015)  # From the data sheet

    def set_temperature_resolution(self, resolution):
        # Read config register
        config = self.read_config_register()
        config = (config & ~0x0400) | resolution

        self.__HDC1080_fw.write(
            bytearray([HDC1080_CONFIGURATION_REGISTER, config >> 8, 0x00])
        )  # sending config register bytes
        time.sleep(0.015)  # From the data sheet

    def read_battery_status(self):
        # Read config register
        config = self.read_config_register()
        config = config & ~HDC1080_CONFIG_HEATER_ENABLE

        if config == 0:
            return True
        else:
            return False

    def read_manufacturer_id(self):
        self.__HDC1080_fw.write(bytearray([HDC1080_MANUFACTURERID_REGISTER]))
        time.sleep(0.0625)  # From the data sheet

        data = self.__HDC1080_fr.read(2)  # read 2 byte config data

        buf = array.array("B", data)
        return buf[0] * 256 + buf[1]

    def read_device_id(self):
        self.__HDC1080_fw.write(bytearray([HDC1080_DEVICEID_REGISTER]))
        time.sleep(0.0625)  # From the data sheet

        data = self.__HDC1080_fr.read(2)  # read 2 byte config data

        buf = array.array("B", data)
        return buf[0] * 256 + buf[1]

    def read_serial_number(self):
        serial_number = 0

        self.__HDC1080_fw.write(bytearray([HDC1080_SERIALIDHIGH_REGISTER]))
        time.sleep(0.0625)  # From the data sheet
        data = self.__HDC1080_fr.read(2)  # read 2 byte config data
        buf = array.array("B", data)
        serial_number = buf[0] * 256 + buf[1]

        self.__HDC1080_fw.write(bytearray([HDC1080_SERIALIDMID_REGISTER]))
        time.sleep(0.0625)  # From the data sheet
        data = self.__HDC1080_fr.read(2)  # read 2 byte config data
        buf = array.array("B", data)
        serial_number = serial_number * 256 + buf[0] * 256 + buf[1]

        self.__HDC1080_fw.write(bytearray([HDC1080_SERIALIDBOTTOM_REGISTER]))
        time.sleep(0.0625)  # From the data sheet
        data = self.__HDC1080_fr.read(2)  # read 2 byte config data
        buf = array.array("B", data)
        serial_number = serial_number * 256 + buf[0] * 256 + buf[1]

        return serial_number
