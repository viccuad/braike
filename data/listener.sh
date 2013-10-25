#!/bin/bash
# This script starts ino serial for reading the arduino serial port,
# saves the read on test.data, and fires up kst for plotting it.

nohup /usr/bin/kst2 kst_config.kst &

echo /dev/null > test.data
echo "reading serial port"
echo "to stop: ctrl+A,ctrl+X"
ino serial -b 57600 > test.data
 

