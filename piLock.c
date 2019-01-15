#include "piLock.h"

#define GPIO_MEM_FILE "/dev/gpiomem"

GPIO_Handle gpiolib_init_gpio(void)
{
	int fd;
	if ((fd = open(GPIO_MEM_FILE, O_RDWR | O_SYNC)) == -1)
		return NULL;

	GPIO_Handle ret;
	ret = mmap(GPIO_BASE, GPIO_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	close(fd);

	if (ret == MAP_FAILED)
		return NULL;

	return ret;
}

void gpiolib_free_gpio(GPIO_Handle handle)
{
	munmap(handle, GPIO_LEN);
}

void gpiolib_write_reg(GPIO_Handle handle, uint32_t offst, uint32_t data)
{
	*(handle + offst) = data;
}

uint32_t gpiolib_read_reg(GPIO_Handle handle, uint32_t offst)
{
	return *(handle + offst);
}

/* =================================================
 * This function changes the I/O type of a GPIO pin.
 * The function accepts an integer value for the number
 * of the pin, as well as an integer that specifies if
 * the pin is to be configured as an input or output
 * (1 = output, 0 = input).  If the pin specified does
 * not exist or the I/O specifier parameter is neither
 * 0 or 1, no changes will be made to the I/O types of
 * any pin, and the method will return -1.  If the
 * arguments are valid, the function will return 0.
 *
 * @param:
 * GPIO_Handle
 * pin number (int) - [0 - 53]
 * pin type (int) [ 1 = OUTPUT, 0 = INPUT ]
 *
 * @return: 0 = successful execution, -1 = error
 * ============================================== */

int selectPin(GPIO_Handle gpio, int pinNumber, int pinType)
{
	if ((pinNumber >= 0 && pinNumber <= 53) && (pinType == 0 || pinType == 1) && gpio != NULL)
	{
		uint32_t selectRegister = 0;
		int registerNumber = (pinNumber / 10);

		if (pinType)
		{
			selectRegister = gpiolib_read_reg(gpio, GPFSEL(registerNumber)); // get current value of GPFSELx register
			selectRegister = selectRegister | (1 << (3 * (pinNumber % 10))); // set control bit to 1 for pin N (control field to 001)
			gpiolib_write_reg(gpio, GPFSEL(registerNumber), selectRegister); // apply updated selection register to GPFSELx
		}
		else
		{
			selectRegister = gpiolib_read_reg(gpio, GPFSEL(registerNumber));  // get current value of GPFSELx register
			selectRegister = selectRegister & ~(1 << (3 * (pinNumber % 10))); // set control bit to 0 for pin N (control field to 000)
			gpiolib_write_reg(gpio, GPFSEL(registerNumber), selectRegister);  // apply updated selection register to GPFSELx
		}

		return 0;
	}
	else
	{
		return -1;
	}
}

/* =================================================
 * This function sets the I/O state of a GPIO pin to HIGH.
 * The function accepts an integer value for the number
 * of the pin.  If the pin specified does not exist,
 * no changes will be made to the I/O states of any pin,
 * and the method will return -1.  If the arguments are
 * valid, the function will return 0.
 *
 * @param: GPIO_Handle, pin number (int) - [0 - 53]
 * @return: 0 = successful execution, -1 = error
 * ============================================== */

int setPin(GPIO_Handle gpio, int pinNumber)
{
	if (pinNumber >= 0 && pinNumber <= 53 && gpio != NULL)
	{
		int registerNumber = 0;

		if (pinNumber >= 0 && pinNumber <= 31)
		{
			registerNumber = 0;
		}
		else if (pinNumber >= 32 && pinNumber <= 53)
		{
			registerNumber = 1;
		}

		gpiolib_write_reg(gpio, GPSET(registerNumber), 1 << pinNumber); // set bit for pin N at bit N in GPSETx

		return 0;
	}
	else
	{
		return -1;
	}
}

/* =================================================
 * This function clears the I/O state of a GPIO pin to LOW.
 * The function accepts an integer value for the number
 * of the pin.  If the pin specified does not exist,
 * no changes will be made to the I/O states of any pin,
 * and the method will return -1.  If the arguments are
 * valid, the function will return 0.
 *
 * @param: GPIO_Handle, pin number (int) - [0 - 53]
 * @return: 0 = successful execution, -1 = error
 * ============================================== */

int clearPin(GPIO_Handle gpio, int pinNumber)
{
	if (pinNumber >= 0 && pinNumber <= 53 && gpio != NULL)
	{
		int registerNumber = 0;

		if (pinNumber >= 0 && pinNumber <= 31)
		{
			registerNumber = 0;
		}
		else if (pinNumber >= 32 && pinNumber <= 53)
		{
			registerNumber = 1;
		}

		gpiolib_write_reg(gpio, GPCLR(registerNumber), 1 << pinNumber); // clear bit for pin N at bit N in GPCLRx

		return 0;
	}
	else
	{
		return -1;
	}
}

/* =================================================
 * This function writes the I/O state of a GPIO pin.
 * The function accepts an integer value for the number
 * of the pin.  If the pin specified does not exist,
 * no changes will be made to the I/O states of any pin.
 * Additionally, the function also accepts an integer
 * value specifying the desired state of the GPIO pin.
 * If 1, the pin will be set to HIGH.  If zero, the pin
 * will be set to LOW.  If neither one or zero, no changes
 * will be made to the state of the pin.  If any of the
 * parameters are not valid the method will return -1.
 * If the arguments are valid, the function will return 0.
 *
 * @param:
 * GPIO_Handle
 * pin number (int) - [0 - 53]
 * pin state (1 == HIGH, 0 == LOW)
 *
 * @return: 0 = successful execution, -1 = error
 * ============================================== */

int writePin(GPIO_Handle gpio, int pinNumber, int state)
{
	if ((pinNumber >= 0 && pinNumber <= 53) && (state == 1 || state == 0) && gpio != NULL)
	{
		int registerNumber = 0;

		if (pinNumber >= 0 && pinNumber <= 31)
		{
			registerNumber = 0;
		}
		else if (pinNumber >= 32 && pinNumber <= 53)
		{
			registerNumber = 1;
		}

		if (state == 1)
		{
			gpiolib_write_reg(gpio, GPSET(registerNumber), 1 << pinNumber); // set bit for pin N at bit N in GPSETx
		}
		else
		{
			gpiolib_write_reg(gpio, GPCLR(registerNumber), 1 << pinNumber); // clear bit for pin N at bit N in GPCLRx
		}

		return 0;
	}
	else
	{
		return -1;
	}
}

/* =================================================
 * This reads the state of a GPIO pin.  If the pin is
 * LOW, the fuction will return 0.  If the pin is HIGH,
 * the function will return 1. The function accepts an
 * integer value for the number of the pin.  If the pin
 * specified does not exist, no changes will be made to t
 * he I/O states of any pin, and the method will return -1.
 * Valid arguments will either return 1 or 0 depending on
 * the state of the pin that is read.
 *
 * @param: GPIO_Handle, pin number (int) - [0 - 53]
 * @return: 1,0 = successful execution, -1 = error
 * ============================================== */

int readPin(GPIO_Handle gpio, int pinNumber)
{
	if (pinNumber >= 0 && pinNumber <= 53 && gpio != NULL)
	{
		uint32_t levelRegister = 0;
		int registerNumber = 0;

		if (pinNumber >= 0 && pinNumber <= 31)
		{
			registerNumber = 0;
		}
		else if (pinNumber >= 32 && pinNumber <= 53)
		{
			registerNumber = 1;
		}

		levelRegister = gpiolib_read_reg(gpio, GPLEV(registerNumber)); // get value of GPLEVx register

		if (levelRegister & (1 << pinNumber)) // check if pin N is zero or not
		{
			return 1; // if not zero, return TRUE (pin is HIGH)
		}
		else
		{
			return 0; // if pin is zero, return FALSE (pin is LOW)
		}
	}
	else
	{
		return -1;
	}
}

/* =================================================
 * This function resets the control registers of the
 * GPIO to 0.  This ensures that all pins are set to
 * low and are configured as inputs, allowing for a
 * clean termination of a program. If the GPIO_Handle
 * is NULL, the function will return an error code of
 * -1. If it is not NULL, it will successfully execute
 * and return 0.  Additionally, the function will terminate
 * by unmapping the memory associated with the GPIO pins.
 *
 * @param: GPIO_Handle
 * @return: 0 = successful execution, -1 = error
 * ============================================== */

int freeGPIO(GPIO_Handle gpio)
{
	if (gpio == NULL)
	{
		return -1;
	}
	else
	{
		gpiolib_free_gpio(gpio);
		return 0;
	}
}

void getTime(char *buffer)
{
	struct timeval tv;
	time_t currentTime;

	gettimeofday(&tv, NULL);

	currentTime = tv.tv_sec;

	// set buffer to a string of current date in mm-dd-yyyy
	// and current time in 24 hour notation

	strftime(buffer, 30, "%m-%d-%Y &T.", localtime(&currentTime));
}

int findLength(const char* fileName)
{
	int nameLength = 0;
	while (fileName[nameLength] != 0) // count the length of program name
	{
		nameLength++;
	}
	return nameLength;
}

void copyProgramName(char* programName, const char* fileName)
{
	int i = 0;
	while (fileName[i + 2] != 0) // get rid of ./ at start of file name
	{
		programName[i] = fileName[i + 2];
		i++;
	}
	programName[i] = 0;

	return;
}

int strCompare(char *compare, char *source)
{
	if (compare == NULL || source == NULL)
	{
		return 0;
	}
	int i = 0;

	while (compare[i] == source[i])
	{
		if (compare[i] == 0 && source[i] == 0)
		{
			return 1;
		}
		++i;
	}
	return 0;
}

void strCopy(char *dest, const char *source)
{
	if (source == 0)
	{
		dest = 0;
		return;
	}
	else
	{
		int i = 0;

		while (source[i] != 0)
		{
			dest[i] = source[i];
			++i;
		}

		dest[i] = 0;
		return;
	}
}

void readConfig(FILE *config, int *timeout, int *lockState, char *lockLogFilePath, char *commFilePath, char *statFilePath, char *keyLogFilePath, char *autoLock[], char *autoUnlock[])
{
	enum configReadState
	{
		START,
		DONE,
		COMMENT,
		WHITESPACE,
		PARAMETER,
		TIMEOUT,
		LOCKSTATE,
		KEYLOG,
		LOCKLOG,
		COMMFILE,
		STATFILE,
		AUTOLOCK,
		AUTOUNLOCK
	};
	enum configReadState configRead = START;

	char buffer[1023];
	*timeout = 0;
	*lockState = 0;
	*lockLogFilePath = 0;
	*commFilePath = 0;
	*statFilePath = 0;
	*keyLogFilePath = 0;

	int i = 0;
	int n = 0;
	char input = buffer[i];
	char parameterName[255];

	while (fgets(buffer, 1023, config) != NULL)
	{
		input = buffer[i];
		if (input == 0)
		{
			configRead = DONE;
		}

		switch (configRead)
		{
		case START:

			if (('a' <= input && input <= 'z') || ('A' <= input && 'Z' <= input))
			{
				configRead = PARAMETER;
			}
			else if (input == '\n' || input == ' ')
			{
				configRead = WHITESPACE;
			}
			else if (input == '#')
			{
				configRead = COMMENT;
			}
			else
			{
				configRead = COMMENT;
				// perror("Invalid characters in config file.");
				// return;
			}
			break;

		case COMMENT:
			if (('a' <= input && input <= 'z') || ('A' <= input && 'Z' <= input))
			{
				configRead = PARAMETER;
			}
			else if (input == '\n' || input == ' ')
			{
				configRead = WHITESPACE;
			}
			else
			{
				configRead = COMMENT;
				// perror("Invalid characters in config file.");
				// return;
			}

		case WHITESPACE:
			if (('a' <= input && input <= 'z') || ('A' <= input && 'Z' <= input))
			{
				configRead = PARAMETER;
			}
			else
			{
				configRead = COMMENT;
				// perror("Invalid characters in config file.");
				// return;
			}
			break;

		case PARAMETER:
			if (strCompare("WATCHDOG_TIMEOUT", parameterName))
			{
				configRead = TIMEOUT;
			}
			else if (strCompare("COMMMUNICATION_FILE_PATH", parameterName))
			{
				configRead = COMMFILE;
			}
			else if (strCompare("KEY_LOG_FILE_PATH", parameterName))
			{
				configRead = KEYLOG;
			}
			else if (strCompare("LOCK_LOG_FILE_PATH", parameterName))
			{
				configRead = LOCKLOG;
			}
			else if (strCompare("STAT_FILE_PATH", parameterName))
			{
				configRead = STATFILE;
			}
			else if (strCompare("DEFAULT_LOCK_STATE", parameterName))
			{
				configRead = LOCKSTATE;
			}
			else
			{
				configRead = COMMENT;
				// perror("Invalid characters in config file.");
				// return;
			}

		case TIMEOUT:
		case COMMFILE:
		case KEYLOG:
		case LOCKLOG:
		case STATFILE:

			if (input == '\n' || input == ' ')
			{
				configRead = WHITESPACE;
			}
			else
			{
				configRead = COMMENT;
				// perror("Invalid characters in config file.");
				// return;
			}

		case DONE:
			return;
			break;
		}

		switch (configRead)
		{
		case COMMENT:
			while (input != '\n')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
			}
			break;

		case WHITESPACE:
			while (input == '\n' || input == ' ')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
			}
			break;

		case PARAMETER:

			n = 0;
			input = buffer[i];
			while (input != ' ')
			{
				if (!(('a' <= input && input <= 'z') || ('A' <= input && 'Z' <= input) || ('0' <= input && input <= '9') || input == '_'))
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				parameterName[n] = input;
				++n;
				++i;
				input = buffer[i];
			}
			parameterName[n] = 0;

			while (input != '=')
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			++i;
			break;

		case TIMEOUT:
			input = buffer[i];
			while (!('0' <= input && input <= '9'))
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			while ('0' <= input && input <= '9')
			{
				*timeout = (*timeout * 10) + (buffer[i] - '0');
				++i;
			}
			break;

		case COMMFILE:
			input = buffer[i];
			while (input != '/')
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			n = 0;
			while (input != ' ')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				commFilePath[n] = input;
				++n;
				++i;
			}
			commFilePath[n] = 0;
			break;

		case KEYLOG:
			input = buffer[i];
			while (input != '/')
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			n = 0;
			while (input != ' ')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				keyLogFilePath[n] = input;
				++n;
				++i;
			}
			keyLogFilePath[n] = 0;
			break;

		case LOCKLOG:
			input = buffer[i];
			while (input != '/')
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			n = 0;
			while (input != ' ')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				lockLogFilePath[n] = input;
				++n;
				++i;
			}
			lockLogFilePath[n] = 0;
			break;

		case STATFILE:
			input = buffer[i];
			while (input != '/')
			{
				if (input != ' ')
				{
					configRead = COMMENT;
					// perror("Invalid characters in config file.");
					// return;
					break;
				}
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				++i;
				input = buffer[i];
			}

			n = 0;
			while (input != ' ')
			{
				input = buffer[i];
				if (input == 0)
				{
					configRead = DONE;
					break;
				}
				statFilePath[n] = input;
				++n;
				++i;
			}
			statFilePath[n] = 0;
			break;

		case DONE:
			return;
			break;
		}
	}
}

int main(const int argc, const char* const argv[]){
	//////////////////////////////////////////////////////////////////////////// Declare default variables
	int timeout;
	char lockLogFilePath[255];
	char commFilePath[255];
	char statFilePath[255];

	// Might be editted when reading config, but initialize default values for now
	timeout = DEFAULT_TIMEOUT;
	strCopy(lockLogFilePath, LOCK_LOG_FILE_PATH);
	printf("%s \n", lockLogFilePath);
	strCopy(commFilePath, COMM_FILE_PATH);
	printf("%s \n", commFilePath);
	strCopy(statFilePath, STAT_FILE_PATH);
	printf("%s \n", statFilePath);

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
		return -1;
	}
	else
	{
		perror("The config file has been opened");
		readConfig(config, &timeout, &defLockState, lockLogFilePath, commFilePath, statFilePath, keyLogFilePath, autoLock, autoUnlock);
	}
	fclose(config);
	///////////////////////////////////////////////////////////////////////////////////////////
	return 0;
}