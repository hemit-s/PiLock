#include "piLock.h"
#include <pigpio.h>

// MACRO DECLARATIONS //
#define PRINT_MSG(file, time, programName, outputStr)                   \
	do                                                                  \
	{                                                                   \
		fprintf(logFile, "%s : %s : %s", time, programName, outputStr); \
	} while (0)

// PIN CONSTANTS //
#define SERVO 14
#define PHOTODIODE 24
#define GREEN_LED 15
#define RED_LED 18

// SERVO POSITION FREQUENCY CONSTANTS //
#define LOCKED_FREQUENCY 1050 // may need further calibration
#define UNLOCKED_FREQUENCY 1950

// OUTPUT MESSAGES //
#define LOCK_COMMAND_MESSAGE "Read communication file, received command to LOCK \n"
#define UNLOCK_COMMAND_MESSAGE "Read communication file, received command to UNLOCK \n"
#define LOCKED_MESSAGE "The door has been LOCKED. \n"
#define UNLOCKED_MESSAGE "The door has been UNLOCKED. \n"

// FUNCTION DECLARATIONS //
void lock(GPIO_Handle);
void unlock(GPIO_Handle);
void cleanup(GPIO_Handle);
int readCommunicationFile();
int checkIfPastHour(char *timeToCheck, int latestHour);

// MAIN METHOD //
int main(const int argc, const char *const argv[])
{
	//////////////////////////////////////////////////////////////////////////// Declare default variables
	int timeout;
	char lockLogFilePath[255];
	char commFilePath[255];
	char statFilePath[255];

	// Might be editted when reading config, but initialize default values for now
	timeout = DEFAULT_TIMEOUT;
	strCopy(lockLogFilePath, LOCK_LOG_FILE_PATH);
	strCopy(commFilePath, COMM_FILE_PATH);
	strCopy(statFilePath, STAT_FILE_PATH);

	int length = findLength(argv[0]);
	char programName[length + 1];
	copyProgramName(programName, argv[0]);
	char time[30];

	int defLockState = DEFAULT_LOCK_STATE;
	char* keyLogFilePath = KEY_LOG_FILE_PATH;
	char* autoLock[255];
	char* autoUnlock[255];
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	FILE *config = fopen(CONFIG_FILE_PATH, "r");
	if (!config)
	{
		perror("The config file could not be opened; using default values");
	}
	else
	{
		readConfig(config, &timeout, &defLockState, lockLogFilePath, commFilePath, statFilePath, keyLogFilePath, autoLock, autoUnlock);
	}
	fclose(config);
	///////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////// SET UP WRITING TO LOG FILE //
	FILE *logFile = fopen(lockLogFilePath, "a");
	if (!logFile)
	{
		// Create a new logfile if it does not exist. Then close it, and set it so
		// the file is being appended to

		logFile = fopen(lockLogFilePath, "w");
		fclose(logFile);
		logFile = fopen(lockLogFilePath, "a");

		getTime(time);
		PRINT_MSG(logFile, time, programName, "# A new log file was created\n\n");
	}
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The program has started.");
	PRINT_MSG(logFile, time, programName, "# The log file has been opened.");
	//////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////
	int watchdog;
	if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0)
	{
		printf("Error: Couldn't open watchdog device! %d\n", watchdog);
		return -1;
	}
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog file has been opened\n\n");

	ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);

	//Get the current time
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog time limit has been set\n\n");

	//The value of timeout will be changed to whatever the current time limit of the
	//watchdog timer is
	ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);

	//This print statement will confirm to us if the time limit has been properly
	//changed. The \n will create a newline character similar to what endl does.
	printf("The watchdog timeout is %d seconds.\n\n", timeout);
	////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////// SET UP WRITING TO STATS FILE //
	int numberOfLocks = 0;
	int numberOfUnlocks = 0;
	FILE *statFile = fopen(statFilePath, "a");
	if (!statFile)
	{
		// Create new statfile if it does not exist. Then close it, and set it so
		// the file is being appended to.
		statFile = fopen(statFilePath, "w");
		fclose(statFile);
		statFile = fopen(statFilePath, "a");

		getTime(time);
		PRINT_MSG(logFile, time, programName, "# A new stats file was created\n\n");
	}
	///////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////// INITIALIZE GPIO FOR BOTH LIBRARIES //
	GPIO_Handle gpio;
	gpio = gpiolib_init_gpio();

	if (gpio == NULL)
	{
		getTime(time);
		PRINT_MSG(logFile, time, programName, "GPIO could not be initialized!\n\n");
		return -1;
	}

	gpioInitialise();
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been initialized\n\n");

	// PIN I/O CONFIGURATION //
	selectPin(gpio, GREEN_LED, 1);
	selectPin(gpio, RED_LED, 1);
	selectPin(gpio, PHOTODIODE, 0);
	// PIGPIO will handle the servo pin
	getTime(time);
	PRINT_MSG(logFile, time, programName, "Pin 14, 15, 18 have been set to output\n Pin 23, 24 have been set to input\n\n");

	// INTIALIZE OUTPUT PINS //
	clearPin(gpio, GREEN_LED);
	clearPin(gpio, RED_LED);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN EXECUTION LOOP //
	int currentCommand = 0;

	enum LockState
	{
		START,
		LOCKED,
		UNLOCKED,
		WAITING_TO_LOCK
	};
	enum LockState lockState = START;

	while (1)
	{
		currentCommand = readCommunicationFile(commFilePath); // 1 if lock, 0 if unlock
		getTime(time);

		// CHECK TIME TO SEE IF A DAY HAS PASSED //
		if (checkIfPastHour(time, 23))
		{
			// char* bufferString[50];
			// sprintf(bufferString, "%d locks, %d unlocks", numberOfLocks, numberOfUnlocks)

			fprintf(statFile, "%s : %s : %d locks, %d unlocks", time, programName, numberOfLocks, numberOfUnlocks);

			// reset the counters for the changes for the day
			numberOfLocks = 0;
			numberOfUnlocks = 0;
		}

		if (currentCommand)
		{
			PRINT_MSG(logFile, time, programName, LOCK_COMMAND_MESSAGE);
		}
		else
		{
			PRINT_MSG(logFile, time, programName, UNLOCK_COMMAND_MESSAGE);
		}

		// currentCommand = readPin(gpio, BUTTON);
		// printf("current %d last %d\n", currentCommand, lastButtonValue);

		switch (lockState)
		{
		case START:

			if (currentCommand)
			{
				lockState = WAITING_TO_LOCK;
			}
			else
			{
				unlock(gpio);
				getTime(time);
				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				printf("UNLOCKED\n");
				lockState = UNLOCKED;
				// Initial state, so it should not change the counter of number of locks/unlocks
			}
			break;

		case LOCKED:

			if (!currentCommand)
			{
				unlock(gpio);
				getTime(time);
				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				printf("UNLOCKED\n");
				lockState = UNLOCKED;
				numberOfUnlocks++;
			}
			break;

		case UNLOCKED:

			if (currentCommand)
			{
				if (!readPin(gpio, PHOTODIODE))
				{
					getTime(time);
					PRINT_MSG(logFile, time, programName, "The door is open, waiting until door is closed to lock");
				}
				printf("WAITING_TO_LOCK\n");
				lockState = WAITING_TO_LOCK;
			}
			break;

		case WAITING_TO_LOCK:
			if (!currentCommand) // button is pressed and was not previously pressed
			{
				unlock(gpio);
				getTime(time);
				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				// printf("UNLOCKED\n");
				lockState = UNLOCKED;
				numberOfUnlocks++;
			}

			if (readPin(gpio, PHOTODIODE))
			{
				usleep(1000000);
				lock(gpio);
				getTime(time);
				PRINT_MSG(logFile, time, programName, LOCKED_MESSAGE);
				// printf("LOCKED\n");
				lockState = LOCKED;
				numberOfLocks++;
			}
			break;
		}

		ioctl(watchdog, WDIOC_KEEPALIVE, 0);
		getTime(time);
		PRINT_MSG(logFile, time, programName, "The Watchdog was updated\n\n");
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	write(watchdog, "V", 1);
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was disabled\n\n");

	close(watchdog);
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was closed\n\n");

	cleanup(gpio);
	gpioTerminate();
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lock(GPIO_Handle gpio)
{
	clearPin(gpio, GREEN_LED);
	gpioServo(SERVO, LOCKED_FREQUENCY);
	setPin(gpio, RED_LED);
}

void unlock(GPIO_Handle gpio)
{
	clearPin(gpio, RED_LED);
	gpioServo(SERVO, UNLOCKED_FREQUENCY);
	setPin(gpio, GREEN_LED);
}

void cleanup(GPIO_Handle gpio)
{
	clearPin(gpio, RED_LED);
	clearPin(gpio, GREEN_LED);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int readCommunicationFile(char *commFilePath) // returns 1 if should be locked, 0 otherwise
{
	FILE *commFile = fopen(commFilePath, "r");
	return (fgetc(commFile) - '0');
}

int checkIfPastHour(char *timeToCheck, int latestHour)
{
	char hourToCheck[3];

	// create a character array with the hour part of the timeToCheck
	hourToCheck[0] = timeToCheck[11];
	hourToCheck[1] = timeToCheck[12];
	hourToCheck[2] = '\0';

	int hours = atoi(hourToCheck);

	if (hours > latestHour)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////