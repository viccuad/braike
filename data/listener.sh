#!/bin/bash
# This script starts ino serial for reading the arduino serial port,
# saves the read on test.data, and fires up kst for plotting it.

/usr/bin/kst2 kst_config.kst 1&> /dev/null &

echo /dev/null > test.data
echo "reading serial port"
echo "to stop: ctrl+A,ctrl+X"
# call ino serial from /firmware so it will use ino.ini values:
cd ../firmware
ino serial > ../data/test.data


