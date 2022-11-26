#!/bin/bash
cd /home/pi
nohup python3 sensor_readout_server.py &> readout.log &
