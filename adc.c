#include "adc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <pigpio.h>

#define GPIO_PIN 27

int fd;
// Note PCF8591 defaults to 0x48!
int asd_address = 0x48;
int config_register = 0x01;
int16_t val, conversionCount = 0;
uint32_t previousTick, startTick;
uint8_t writeBuf[3];
uint8_t readConfigBuf[3];
uint8_t readBuf[2];
float myfloat;
const float VPS = 4.096 / 32768.0; // volts per step

typedef struct EcgPoint {
    int X;
    int Y;
};

static void ReadConversionRegister()
{
    // read conversion register
	if (read(fd, readBuf, 2) != 2) {
		perror("Read conversion");
		exit(-1);
	}

	// could also multiply by 256 then add readBuf[1]
	val = readBuf[1] << 8 | readBuf[0];

	// with +- LSB sometimes generates very low neg number.
	if (val < 0)   
		val = 0;

	myfloat = val * VPS; // convert to voltage
}

static void SetupConversionReadyCallback(void *ConvertionCallback)
{
    if (gpioInitialise() < 0)
	{
		fprintf(stderr, "pigpio initialisation failed\n");
		exit(1);
	}

	gpioSetAlertFuncEx(GPIO_PIN, OnConversionReady, ConvertionCallback);
	gpioSetMode(GPIO_PIN, PI_INPUT);
}

void Initialize_ADS1115()
{
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
		printf("Error: Couldn't open device! %d\n", fd);
		exit (1);
	}

	int res, retValue;
	// connect to ADS1115 as i2c slave
	if (res = ioctl(fd, I2C_SLAVE, asd_address) < 0) {
		printf("Error: Couldn't find device on address!\n");
		exit (1);
	}

	// set config register and start conversion
	// AIN0 and GND, 4.096v, 128s/s
	// Refer to page 19 area of spec sheet
	writeBuf[0] = 1; // config register is 1
	writeBuf[1] = 0b11000010; // 0xC2 single shot off
	// bit 15 flag bit for single shot not used here
	// Bits 14-12 input selection:
	// 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
	// Bits 11-9 Amp gain. Default to 010 here 001 P19
	// Bit 8 Operational mode of the ADS1115.
	// 0 : Continuous conversion mode
	// 1 : Power-down single-shot mode (default)

	writeBuf[2] = 0b11100101; // bits 7-0  0x85
	// Bits 7-5 data rate default to 100 for 128SPS
	// Bits 4-0  comparator functions see spec sheet.
	
	if (retValue = write(fd, writeBuf, 3) != 3) {
		perror("Write to register 1");
		exit (1);
	}
	
	writeBuf[0] = 0b00000010; // Lo-thrash register is 0b00000010
	writeBuf[1] = 0b00000000; // MSB 0
	writeBuf[2] = 0b00000000; 
	if (retValue = write(fd, writeBuf, 3) != 3) {
		perror("Write to register 1");
		exit (1);
	}
	
	writeBuf[0] = 0b00000011; // Hi-thrash register is 0b00000011
	writeBuf[1] = 0b10000000; // MSB 1
	writeBuf[2] = 0b00000000; 
	if (retValue = write(fd, writeBuf, 3) != 3) {
		printf("Value returned: %d\n", retValue);
		perror("Write to register 1");
		exit (1);
	}
}

void OnConversionReady(int gpio, int level, uint32_t tick, void *data)
{
	if(level == 0) return;
	
	if(startTick==0)
	{
		startTick = tick;
		conversionCount = 0;
	}
	
	if((tick - startTick) > 1000000)
	{
		printf("Conversions per second: %d\n", conversionCount);
		startTick = tick;
		conversionCount = 0;
	}
	else
	{
		conversionCount++;
	}
	
	ReadConversion(conversionCount, tick - startTick);
	previousTick = tick;
}
