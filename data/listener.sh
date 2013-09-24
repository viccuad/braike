#!/bin/sh
# This script configures the stty for reading the arduino serial port,
# starts listening on it and saves the output on textdata.data

stty -F /dev/ttyACM0 57600
cat /dev/ttyACM0 > testdata.txt

