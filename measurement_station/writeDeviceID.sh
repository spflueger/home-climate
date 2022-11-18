
#/bin/sh

echo "Enter Device ID to write to EEPROM:"
read deviceID
echo "Using device id $deviceID"
avrdude $@ -U eeprom:w:0x$deviceID:m