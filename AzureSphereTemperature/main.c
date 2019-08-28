#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "applibs_versions.h"
#include <applibs/log.h>

#include "mt3620_rdb.h"

// 1. include the Grove, Grove LED Button, and Grove Temp Humidity Headers from the MT3620_Grove_Shield_Libary
#include "Grove.h"
#include "Sensors/GroveLEDButton.h"
#include "Sensors/GroveTempHumiSHT31.h"
#include "Sensors/GroveOledDisplay96x96.h"	

static volatile sig_atomic_t terminationRequested = false;

void i2cScanner(int i2cFd)
{
	uint8_t i2cState;
	const struct timespec sleepTime = { 0, 10000000 };

	for (uint8_t addr = 1; addr < 127; addr++)
	{
		GroveI2C_WriteBytes(i2cFd, (uint8_t)(addr << 1), NULL, 0);
		SC18IM700_ReadReg(i2cFd, 0x0A, &i2cState);
		if (i2cState == I2C_OK)
		{
			Log_Debug("I2C_OK, address detect: 0x%02X\r\n", addr);
		}

		nanosleep(&sleepTime, NULL);
	}
}

static void TerminationHandler(int signalNumber)
{
	terminationRequested = true;
}

int main(int argc, char *argv[])
{
	// 3. initialize i2c feed with baud rate and temp humidity sensor
	int i2cFeed;
	GroveShield_Initialize(&i2cFeed, 230400);
	void* tempHumiditySensor = GroveTempHumiSHT31_Open(i2cFeed);

	/** Initialize OLED */
	GroveOledDisplay_Init(i2cFeed, SH1107G);

	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	clearDisplay();
	setNormalDisplay();
	setVerticalMode();

	// FYI - loop is every 1 secounds
	const int sleepTime = 1000;
	while (!terminationRequested) {

		GroveTempHumiSHT31_Read(tempHumiditySensor);
		float temperature = GroveTempHumiSHT31_GetTemperature(tempHumiditySensor);
		float humidity = GroveTempHumiSHT31_GetHumidity(tempHumiditySensor);

		Log_Debug("Temperature: %.1fC\n", temperature);
		Log_Debug("Humidity: %.1f\%c\n", humidity, 0x25);

		clearDisplay();

		setTextXY(0, 0); 
		putString("Temperature: ");
		putNumber(temperature);

		setTextXY(1, 0);
		putString("Humidity: ");
		putNumber(humidity);
		putString("%");

#ifdef _WIN32
		Sleep(sleepTime);
#else
		usleep(sleepTime * 1000);  /* sleep for 100 milliSeconds */
#endif
	}
	return 0;
}