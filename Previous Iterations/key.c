#include "piLock.h"

// MACRO DECLARATIONS //

#define PRINT_MSG(file, time, programName, outputStr)                   \
	do                                                                  \
	{                                                                   \
		fprintf(logFile, "%s : %s : %s", time, programName, outputStr); \
	} while (0)

// PIN CONSTANTS //
#define BUTTON 14

int main(const int argc, const char *const argv[])
{
	/////////////////////////////////////////////////////////////////////////////////////////////////// Declare default variables
	int timeout;
	int lockState;
	char keyLogFilePath[255];
	char commFilePath[255];

	// Might be editted when reading config, but initialize default values for now
	timeout = DEFAULT_TIMEOUT;
	lockState = DEFAULT_LOCK_STATE;
	strCopy(keyLogFilePath, LOCK_LOG_FILE_PATH);
	strCopy(commFilePath, COMM_FILE_PATH);

	char *lockLogFilePath = LOCK_LOG_FILE_PATH;
	char *statFilePath = STAT_FILE_PATH;
	char *autoLock[255];
	char *autoUnlock[255];

	int length = findLength(argv[0]);
	char programName[length + 1];
	copyProgramName(programName, argv[0]);
	char time[30];
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	FILE *config = fopen(CONFIG_FILE_PATH, "r");
	if (!config)
	{
		perror("The config file could not be opened; using default values");
	}
	else
	{
//		readConfig(config, &timeout, &lockState, lockLogFilePath, commFilePath, statFilePath, keyLogFilePath, autoLock, autoUnlock);
	}
	fclose(config);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////// SET UP WRITING TO LOG FILE //
	FILE *logFile = fopen(keyLogFilePath, "a");
	if (!logFile)
	{
		// Create a new logfile if it does not exist. Then close it, and set it so
		// the file is being appended to

		logFile = fopen(keyLogFilePath, "w");
		fclose(logFile);
		logFile = fopen(keyLogFilePath, "a");

		getTime(time);
		PRINT_MSG(logFile, time, programName, "A new log file was created\n\n");
	}
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The program has started.");
	PRINT_MSG(logFile, time, programName, "The log file has been opened.");
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int watchdog;
	if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0)
	{
		printf("Error: Couldn't open watchdog device! %d\n", watchdog);
		return -1;
	}
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog file has been opened\n\n");

	ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);

	//Get the current time
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog time limit has been set\n\n");

	//The value of timeout will be changed to whatever the current time limit of the
	//watchdog timer is
	ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);

	//This print statement will confirm to us if the time limit has been properly
	//changed. The \n will create a newline character similar to what endl does.
	printf("The watchdog timeout is %d seconds.\n\n", timeout);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	FILE *commFile = fopen(commFilePath, "w");
	if (!commFile)
	{
		// Create a new commFile if it does not exist. Then close it, and set it so
		// the file is being appended to

		commFile = fopen(commFilePath, "w");
		fclose(commFile);
		commFile = fopen(commFilePath, "a");

		getTime(time);
		PRINT_MSG(logFile, time, programName, "A new communication file was created\n\n");
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////////// INITIALIZE GPIO //
	GPIO_Handle gpio;
	gpio = gpiolib_init_gpio();
	if (gpio == NULL)
	{
		getTime(time);
		PRINT_MSG(logFile, time, programName, "GPIO could not be initialized!\n\n");
	}

	// PIN I/O CONFIGURATION //
	selectPin(gpio, BUTTON, 0);
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been initialized\n\n");
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN EXECUTION LOOP //
	int currentButtonValue = readPin(gpio, BUTTON);
	int lastButtonValue = readPin(gpio, BUTTON);

	enum buttonState
	{
		START,
		PRESSED,
		UNPRESSED
	};
	enum buttonState buttonState = START;

	while (1)
	{
		currentButtonValue = readPin(gpio, BUTTON);

		switch (buttonState)
		{
		case START:
			if (currentButtonValue)
			{
				buttonState = PRESSED;
			}
			else
			{
				buttonState = UNPRESSED;
			}
			break;

		case PRESSED:

			if (!currentButtonValue && lastButtonValue) // button is NOT pressed and was previously pressed
			{
				if (lockState == 0)
				{
					lockState = 1;
					fprintf(commFile, "1");
					getTime(time);
					PRINT_MSG(logFile, time, programName, "Wrote command to lock the door to communication file");
				}
				else /*if (lockState == LOCKED)*/
				{
					lockState = 0;
					fprintf(commFile, "0");
					getTime(time);
					PRINT_MSG(logFile, time, programName, "Wrote command to unlock the door to communication file");
				}

				buttonState = UNPRESSED;
			}
			break;

		case UNPRESSED:

			if (currentButtonValue && !lastButtonValue) // button is pressed and was NOT previously pressed
			{
				buttonState = PRESSED;
			}
			break;
		}

		lastButtonValue = currentButtonValue;

		ioctl(watchdog, WDIOC_KEEPALIVE, 0);
		getTime(time);
		PRINT_MSG(logFile, time, programName, "# The Watchdog was updated\n\n");
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	write(watchdog, "V", 1);
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog was disabled\n\n");

	close(watchdog);
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog was closed\n\n");
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}