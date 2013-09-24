Braike
======

Braike: the rear bike light that brights up automatically when you brake. like you car.

# How it works

The idea is to use an accelerometer to read the movement, and light the LED when
braking. Also, thanks to the accel, it is possible to implement a low power
mode: the light will switch itself off if not moved for a while.

# Milestones


 - [x] Set the accelerometer and be able to receive from it. Understand the accel.
 - [x] Set up simple LED. Have a LED bright up when the accel detects the "bike" is braking.
 - [ ] Filter the accel axis info. Differenciate from humps and lateral movements, and braking (the though one).
 - [ ] Set up powerful final LED.
 - [ ] Power it by batteries.
 - [ ] Go to low power mode on atmega and accelerometer if there isn't movement for 1 minute. Set up wake-up.
 - [ ] End prototype phase: abandon Arduino format, program an ATmega328. Design final HW format.
 - [ ] Pack it for use on the bike!
 - [ ] ?
 - [ ] Profit!

# Building
Do it with Arduino IDE or go console style (preferred way):

1. Install [inotool](http://inotool.org/)
2. Go to firmware directory and then `ìnotool build` & `inotool upload`


# Improvements

Add a photoresistor to modulate the brightness of the LED, and extend the battery.

# Libraries 
 * I2C library. Although it seems that the 'wire' Arduino library
supports the repeated start and now interfaces correctly with the MMA8452Q
accel.

# License

![gplv3](https://github.com/viccuad/braike/raw/master/assets/web/gplv3.png)

This work is released under the terms of GPLv3 license. You can find a copy of
the GPLv3 license in LICENSE file.
