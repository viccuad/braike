/*
 Hardware setup:
 MMA8452Q Breakout ------- Arduino
 3.3V --------------------- 3.3V
 SDA ----------------------- A4
 SCL ----------------------- A5
 INT2 ---------------------- D3
 INT1 ---------------------- D2
 GND ---------------------- GND

 SDA and SCL should have external pull-up resistors (to 3.3V).
 10k resistors worked for me. They should be on the breakout
 board.

 Note: The MMA8452 is an I2C sensor, however this code does
 not make use of the Arduino Wire library. Because the sensor
 is not 5V tolerant, we can't use the internal pull-ups used
 by the Wire library. Instead use the included i2c.h, defs.h and types.h files.
 */
#include "i2c.h"  // not the wire library, can't use pull-ups

// The MMA8452Q breakout board defaults to 1, set to 0 if SA0 jumper on the bottom of the board is set
#define SA0 1
#if SA0
#define MMA8452_ADDRESS 0x1D  // SA0 is high, 0x1C if low
#else
#define MMA8452_ADDRESS 0x1C
#endif

/* Set the scale to either 2 (+/-2), 4 or 8g */
const byte GSCALE = 2;
/* Set the output data rate. Value should be between 0 and 7 */
const byte dataRate = 4;  // 0=800Hz, 1=400, 2=200, 3=100, 4=50, 5=12.5, 6=6.25, 7=1.56

/* Pin definitions */
int int1Pin = 2;    // These can be changed, 2 and 3 are the Arduinos external interruption pins
int ledPin  = 9;    // digital pin where the LED is connected. Connect to a pin that was pulse width modulation to fake analog output.

/* Led variables */
int LEDstate = LOW;             // LEDstate used to set the LED
long previousMillis = 0;        // will store last time LED was updated
int resting_brightness = 0;             // how bright the LED is. value [0,255]
// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
const long intervalLED = 1000;  // interval at which to blink (milliseconds)
long LEDonMillis;               // will store last time the led time was updated

/* Sleep variables */
const long intervalSleep = 60000;  // interval at which to go to sleep (milliseconds)

/* Accel variables */
int accelCount[3];  // Stores the 12-bit signed value
float accelG[3];    // Stores the real accel value in g's

/* Moving average variables */
float Sample_Vector[9];
float subtotal, min, max, acc;

/* Threshold variables */
bool brakeLed = false;
float soft_braking, medium_braking, strong_braking, maximum_value;


void setup()
{
	byte c;

	/* Debug printing */
	Serial.begin(9600);

	/* Set up the interrupt pins, they're set as active high, push-pull */
	pinMode(int1Pin, INPUT);
	digitalWrite(int1Pin, LOW);
	pinMode(ledPin, OUTPUT);

	/* Initialize LED */
	analogWrite(ledPin,resting_brightness);

	/* Read the WHO_AM_I register, this is a good test of communication */
	c = readRegister(0x0D);  // read WHO_AM_I register
	if (c == 0x2A) {// WHO_AM_I should always be 0x2A
		initMMA8452(GSCALE, dataRate);  // init the accelerometer if communication is OK
		/* Debug printing */
		Serial.println("MMA8452Q is online...");
	}
	else {
		/* Debug printing */
		Serial.print("Could not connect to MMA8452Q: 0x");
		Serial.println(c, HEX);
		while(1) ; // loop forever if communication doesn't happen
	}

	/* Set up thresholds */
	maximum_value = 2;
	soft_braking = 0.3 * maximum_value;
	medium_braking = 0.5 * maximum_value;
	strong_braking = 0.7 * maximum_value;
}

/**
 * Main loop
 */
void loop()
{
	static byte source;

	// If int1 goes high, all data registers have new data
	if (digitalRead(int1Pin)==1) { // Interrupt pin, should probably attach to interrupt function

		/* Digital filtering using a moving average as a low-pass filter */
		subtotal = 0;
		for(int k=0; k<10; ++k){
			if(k!=9)
				Sample_Vector[k] = Sample_Vector[k+1];
			else {
				/* When new data available, read the x/y/z adc values */
				readAccelData(accelCount);

				/* Now we'll calculate the accleration value into actual g's.
				   because of the moving average, their amplitude is reduced */
				for (int i=0; i<3; i++)
					accelG[i] = (float) accelCount[i]/((1<<12)/(2*GSCALE));

				Sample_Vector[9] = accelG[2]; // Z axis

			}
			subtotal += Sample_Vector[k];

			/* Calculation of the max and the min in the buffer vector */
			if( Sample_Vector[k] < min )
				min = Sample_Vector[k];
			if( Sample_Vector[k] > max )
				max = Sample_Vector[k];
		}
		/* Calculation of the moving average value */
		acc = ((subtotal - min - max)/8);

		/* Debug printing */
		Serial.print(Sample_Vector[9]);
		Serial.print("\t");  // tabs in between axes
		Serial.print(acc);
		Serial.print("\t");  // tabs in between axes
		Serial.print(1);
		Serial.println();

		/* Lookup activation thresholds */
		if(acc > strong_braking){ 		/* it corresponds to the maximum braking effort */
    			analogWrite(ledPin, 64); 	/* set the PWM duty cycle to 25% */
			brakeLed = TRUE;
		}
		else if(acc > medium_braking){		/* it corresponds to a medium braking effort */
			analogWrite(ledPin, 127); 	/* set the PWM duty cycle to 50% */
			brakeLed = TRUE;
		}
		else if(acc > soft_braking){		/* it corresponds to a soft braking effort */
			analogWrite(ledPin, 191);	/* set the PWM duty cycle to 75% */
			brakeLed = TRUE;
		}
		else{ 					/* if no braking */
			analogWrite(ledPin, 255); 	/* PWM duty cycle to 100% set LEDs OFF */
			brakeLed = FALSE;
		}
		checkTimers();
	}
}


/*
 * Lights the LED if order brakeLed is received,
 * powers it off after intervalLED milliseconds.
 *
 * Makes all go to sleep if no movement is received in intervalSleep millis.
 *
 */
void checkTimers()
{
	/* Blink sincHoriario LED */
	if (brakeLed){// if time sync frame, light up led
		LEDonMillis = millis();
		brakeLed = false;
	}
	/* Check to see if it's time to power off the LED;
	   that is, if the difference between the current time and last time you
	   blinked the LED is bigger than the interval at which you want to have
	   the LED on */
	unsigned long currentMillis = millis();
	if(currentMillis - LEDonMillis > intervalLED) {
		// save the last time you blinked the LED
		analogWrite(ledPin, resting_brightness); // default brightness
	}
}



/**
 * Retrieves Accel Data and calculates g values
 */
void readAccelData(int * destination)
{
	byte rawData[6];  // x/y/z accel register data stored here

	readRegisters(0x01, 6, &rawData[0]);  // Read the six raw data registers into data array

	/* Loop to calculate 12-bit ADC and g value for each axis */
	for (int i=0; i<6; i+=2) {
		destination[i/2] = ((rawData[i] << 8) | rawData[i+1]) >> 4;  // Turn the MSB and LSB into a 12-bit value
		if (rawData[i] > 0x7F) {
			/* If the number is negative, we have to make it so manually (no 12-bit data type) */
			destination[i/2] = ~destination[i/2] + 1;
			destination[i/2] *= -1;  // Transform into negative 2's complement #
		}
	}
}

/**
 * Initialize the MMA8452 registers
 * See the many application notes for more info on setting all of these registers
 * on the MMA8452Q datasheet.
 * Feel free to modify any values, these are settings that work well for me.
 */
void initMMA8452(byte fsr, byte dataRate)
{
	MMA8452Standby();  // Must be in standby to change registers

	/* Set up the full scale range to 2, 4, or 8g */
	if ((fsr==2)||(fsr==4)||(fsr==8))
		writeRegister(0x0E, fsr >> 2);
	else
		writeRegister(0x0E, 0);

	/* Setup the 3 data rate bits, from 0 to 7 */
	writeRegister(0x2A, readRegister(0x2A) & ~(0x38));
	if (dataRate <= 7)
		writeRegister(0x2A, readRegister(0x2A) | (dataRate << 3));

	/* Set up portrait/landscap registers - 4 steps:
	   1. Enable P/L
	   2. Set the back/front angle trigger points (z-lock)
	   3. Set the threshold/hysteresis angle
	   4. Set the debouce rate
	   For more info check out the MMA8452Q data sheet */
	writeRegister(0x11, 0x40);  // 1. Enable P/L
	writeRegister(0x13, 0x44);  // 2. 29deg z-lock (don't think this register is actually writable)
	writeRegister(0x14, 0x84);  // 3. 45deg thresh, 14deg hyst (don't think this register is writable either)
	writeRegister(0x12, 0x50);  // 4. debounce counter at 100ms (at 800 hz)

	/* Set up single and double tap - 5 steps:
		1. Set up single and/or double tap detection on each axis individually.
		2. Set the threshold - minimum required acceleration to cause a tap.
		3. Set the time limit - the maximum time that a tap can be above the threshold
		4. Set the pulse latency - the minimum required time between one pulse and the next
		5. Set the second pulse window - maximum allowed time between end of latency and start of second pulse
	   for more info check out this app note: http://cache.freescale.com/files/sensors/doc/app_note/AN4072.pdf */
	writeRegister(0x21, 0x7F);  // 1. enable single/double taps on all axes
	// writeRegister(0x21, 0x55);  // 1. single taps only on all axes
	// writeRegister(0x21, 0x6A);  // 1. double taps only on all axes
	writeRegister(0x23, 0x20);  // 2. x thresh at 2g, multiply the value by 0.0625g/LSB to get the threshold
	writeRegister(0x24, 0x20);  // 2. y thresh at 2g, multiply the value by 0.0625g/LSB to get the threshold
	writeRegister(0x25, 0x08);  // 2. z thresh at .5g, multiply the value by 0.0625g/LSB to get the threshold
	writeRegister(0x26, 0x30);  // 3. 30ms time limit at 800Hz odr, this is very dependent on data rate, see the app note
	writeRegister(0x27, 0xA0);  // 4. 200ms (at 800Hz odr) between taps min, this also depends on the data rate
	writeRegister(0x28, 0xFF);  // 5. 318ms (max value) between taps max

	/* Set up interrupt 1 and 2 */
	writeRegister(0x2C, 0x02);  // Active high, push-pull interrupts
	writeRegister(0x2D, 0x19);  // DRDY, P/L and tap ints enabled
	writeRegister(0x2E, 0x01);  // DRDY on INT1, P/L and taps on INT2

	MMA8452Active();  // Set to active to start reading
}

/**
 * Sets the MMA8452 to standby mode.
 * It must be in standby to change most register settings
 */
void MMA8452Standby()
{
	byte c = readRegister(0x2A);
	writeRegister(0x2A, c & ~(0x01));
}

/**
 * Sets the MMA8452 to active mode.
 * Needs to be in this mode to output data
 */
void MMA8452Active()
{
	byte c = readRegister(0x2A);
	writeRegister(0x2A, c | 0x01);
}

/**
 * Read i registers sequentially, starting at address into the dest byte array
 */
void readRegisters(byte address, int i, byte * dest)
{
	i2cSendStart();
	i2cWaitForComplete();

	i2cSendByte((MMA8452_ADDRESS<<1)); // write 0xB4
	i2cWaitForComplete();

	i2cSendByte(address);	// write register address
	i2cWaitForComplete();

	i2cSendStart();
	i2cSendByte((MMA8452_ADDRESS<<1)|0x01); // write 0xB5
	i2cWaitForComplete();
	for (int j=0; j<i; j++) {
		i2cReceiveByte(TRUE);
		i2cWaitForComplete();
		dest[j] = i2cGetReceivedByte(); // Get MSB result
	}
	i2cWaitForComplete();
	i2cSendStop();

	cbi(TWCR, TWEN); // Disable TWI
	sbi(TWCR, TWEN); // Enable TWI
}

/**
 * Read a single byte from address and return it as a byte
 */
byte readRegister(uint8_t address)
{
	byte data;

	i2cSendStart();
	i2cWaitForComplete();

	i2cSendByte((MMA8452_ADDRESS<<1)); // Write 0xB4
	i2cWaitForComplete();

	i2cSendByte(address);	// Write register address
	i2cWaitForComplete();

	i2cSendStart();

	i2cSendByte((MMA8452_ADDRESS<<1)|0x01); // Write 0xB5
	i2cWaitForComplete();
	i2cReceiveByte(TRUE);
	i2cWaitForComplete();

	data = i2cGetReceivedByte();	// Get MSB result
	i2cWaitForComplete();
	i2cSendStop();

	cbi(TWCR, TWEN);	// Disable TWI
	sbi(TWCR, TWEN);	// Enable TWI

	return data;
}

/**
 * Writes a single byte (data) into address
 */
void writeRegister(unsigned char address, unsigned char data)
{
	i2cSendStart();
	i2cWaitForComplete();

	i2cSendByte((MMA8452_ADDRESS<<1)); // Write 0xB4
	i2cWaitForComplete();

	i2cSendByte(address);	// Write register address
	i2cWaitForComplete();

	i2cSendByte(data);
	i2cWaitForComplete();

	i2cSendStop();
}


