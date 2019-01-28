/* =================================================
 * Authors: Kyle Pinto, Hemit Shah, Efaz Shikder
 * Date: December 03, 2018.
 * UW ID: 20772174, 20756780, 20778157
 * UserIDs: krpinto, h39shah, eashikde
 * -------------------------------------------------
 * HARDWARE CONFIGURATION:
 * 
 * OUTPUTS: SERVO (GPIO 14), GREEN_LED (GPIO 15), RED_LED (GPIO 18)
 * INPUTS: PHOTODIODE (GPIO 24), BUTTON (GPIO 23)
 * -------------------------------------------------
 * PROGRAM DESCRIPTION:
 *
 * This program consists of a simple state machine 
 * that implements PIGPIO library functions to control 
 * the servo motor and a function to read the 
 * communication file stored in the shared network
 * folder. GPIO library functions are also used to
 * control the LED and button circuits. Reduced to a
 * few lines, the program running on the lock 
 * mechanism reads the current command (a ‘1’ or ‘0’ 
 * specifying locked or unlocked respectively) in 
 * commFile.txt and executes the command through a state 
 * machine that consists of four major states: START, 
 * UNLOCKED, WAITING_TO_LOCK (waiting for the door 
 * to close as detected by the photodiode) and LOCKED.
 * 
 * ============================================== */

// Import the necessary header files
#include "piLock.h"
#include <pigpio.h>

// PIN CONSTANTS FOR HARDWARE ELEMENTS CONNECTED TO GPIO PINS ON LOCK MECHANISM PI//
#define SERVO 14
#define BUTTON 23
#define PHOTODIODE 24
#define GREEN_LED 15
#define RED_LED 18

// SERVO POSITION FREQUENCY CONSTANTS //
#define LOCKED_FREQUENCY 1050 // may need further calibration
#define UNLOCKED_FREQUENCY 1950

// Default messages for logging purposes after reading commands from communication file and after executing commands //
#define LOCK_COMMAND_MESSAGE "Read communication file, received command to LOCK \n"
#define UNLOCK_COMMAND_MESSAGE "Read communication file, received command to UNLOCK \n"
#define LOCKED_MESSAGE "The door has been LOCKED. \n"
#define UNLOCKED_MESSAGE "The door has been UNLOCKED. \n"

// SERVO/LED CONTROL FUNCTION DECLARATIONS //
void lock(GPIO_Handle);
void unlock(GPIO_Handle);
void cleanup(GPIO_Handle);

// MAIN METHOD //
int main(const int argc, const char *const argv[])
{
	
	///////////////////////////////////////////////////////////////////////////////////// DEFAULT VARIABLE INITIALIZATION //
	//Declare default variables and file paths that will be read from the configuration file
	int timeout;					// Watchdog timout
	int lockState;					// Initial lock state (1 for locked, 0 for unlocked)

	char commFilePath[255];			// File path to the communication file shared between Pis
	char lockLogFilePath[255];		// File path to the log file for the lock mechanism
	char keyLogFilePath[255];		// File path to the log file for the remote access key

	// Initialize default variables and file paths to those defined in the piLock.h header file
	timeout = DEFAULT_TIMEOUT;
	lockState = DEFAULT_LOCK_STATE;
	
	// Use the strCopy() function to copy the strings rather than point to the default strings in piLock.h
	strCopy(commFilePath, COMM_FILE_PATH);
	strCopy(lockLogFilePath, LOCK_LOG_FILE_PATH);
	strCopy(keyLogFilePath, KEY_LOG_FILE_PATH);

	// Extract the name of the program (while removing the "./") using findLength() and copyProgramName() for logging purposes later on
	int length = findLength(argv[0]);
	char programName[length + 1];
	copyProgramName(programName, argv[0]);	char time[30]; 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////////////// READING FROM CONFIG FILE //
	// Open config file if possible invoking fopen() with "r" to set as a read-only file. 
	// If config cannot be opened output a message to the user that the default values declared in the header will be used
	FILE *config = fopen(CONFIG_FILE_PATH, "r");
	if (!config)
	{
		perror("The config file could not be opened; using default values");
	}
	else
	{
		// Read the configuration file once opened and 
		readConfig(config, &timeout, &lockState, commFilePath, lockLogFilePath, keyLogFilePath);
	}
	fclose(config);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////////////// SET UP WRITING TO LOG FILE //
	// Attempt to open log file invoking fopen() with "r" to see if the file exists
	FILE *logFile = fopen(lockLogFilePath, "r");
	if (!logFile)
	{
		// If the file does not exist, create a new file at the lockLogFilePath (either default or configuration based)
		// by invoking fopen() with "a" and immediately write a message that a new log file was created
		logFile = fopen(lockLogFilePath, "a");
		getTime(time);
		PRINT_MSG(logFile, time, programName, "# A new log file was created\n\n");
		fclose(logFile);
	}
	fclose(logFile);
	logFile = fopen(lockLogFilePath, "a");
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The log file has been opened.\n\n");
	PRINT_MSG(logFile, time, programName, "# The program has started. \n\n");
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////////////////////////// OPEN COMM FILE //
	// Attempt to open log file invoking fopen() with "r" to see if the file exists
	FILE *commFile = fopen(commFilePath, "r");
	if (!commFile)
	{
		// If the comm file does not exist, write to the log file that a comm file was created
		getTime(time);
		PRINT_MSG(logFile, time, programName, "A new communication file was created\n\n");
	}
	fclose(commFile);
	// Open comm file using fopen() with "w". A new file will automatically be created at commFilePath (default or configuration based)
	// if it does not exist
	commFile = fopen(commFilePath, "w");
	fclose(commFile);

	// Print message to the log file that the communication file was successfuly opened
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The communication file has been opened.\n\n");
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	///////////////////////////////////////////////////////////////////////////////////////////// WATCHDOG INITIALIZATION //
	int watchdog;
	if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0)
	{
		printf("Error: Couldn't open watchdog device! %d\n", watchdog);
		return -1;
	}
	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog file has been opened\n\n");

	ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);

	getTime(time);
	PRINT_MSG(logFile, time, programName, "# The Watchdog time limit has been set\n\n");

	//The value of timeout will be changed to whatever the current time limit of the watchdog timer is
	ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////// INITIALIZE GPIO FOR BOTH LIBRARIES //
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

	// SET PIN I/O CONFIGURATION //
	selectPin(gpio, GREEN_LED, 1);
	selectPin(gpio, RED_LED, 1);
	selectPin(gpio, BUTTON, 0);
	selectPin(gpio, PHOTODIODE, 0);
	// PIGPIO will handle the servo pin
	getTime(time);
	PRINT_MSG(logFile, time, programName, "Pin 14, 15, 18 have been set to output\n Pin 23, 24 have been set to input\n\n");
	// INTIALIZE OUTPUT PINS //
	clearPin(gpio, GREEN_LED);
	clearPin(gpio, RED_LED);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Close logFile before entering main execution loop to fully write messages to log file
	fclose(logFile);
	

	////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN EXECUTION LOOP //
	// Initialize the integer currentCommand which stores the value of the integer in the communication file
	int currentCommand = 0;

	// Create enum LockState for use in the state machine controlling the lock mechanism
	enum LockState
	{
		START,
		LOCKED,
		UNLOCKED,
		WAITING_TO_LOCK
	};
	enum LockState lockState = START;

	// Initialize the integer currentButtonValue (stores the current state of the pin connected to the button)
	int currentButtonValue = readPin(gpio, BUTTON);
	// Initialize the integer lastButtonValue (stores the previous state of the pin connected to the button)
	// Used to compare with currentButtonValue to determine if the button state has changed or not
	int lastButtonValue = readPin(gpio, BUTTON);

	// Create and enum buttonState for use in the state machine receiving input from the button
	// and controlling the commands written to the communication file
	enum buttonState
	{
		START,
		PRESSED,
		UNPRESSED
	};
	enum buttonState buttonState = START;

	// Enter main execution loop in which the button state will continuously be read
	// and the lock mechanism will continously be controlled based on the command stored in the communication file
	while (1)
	{
		
		currentButtonValue = readPin(gpio, BUTTON);	// read the current state of the pin connected to the button

		switch (buttonState)
		{
		case START:
			if (currentButtonValue)					
			{
				buttonState = PRESSED;		// Initial case transition to PRESSED if the button pin reads HIGH 
			}
			else
			{
				buttonState = UNPRESSED;	// Else if button is not pressed transition to UNPRESSED
			}
			break;

		case PRESSED:

			if (!currentButtonValue && lastButtonValue) 	// button was pressed in previous loop and has been released -> write a command to the communication file based on the previous state of the
			{
				if (readCommunicationFile(commFilePath))	// if the previous command stored in the communication file is a 1 (locked)
				{
					commFile = fopen(commFilePath, "w");	// Open the communication file
                    			fprintf(commFile, "0");			// Write a 0 to the communication file indicating that the new command is to unlock the door
					fclose(commFile);			// Close the communication file to finish writing

					// Write to the log file that a command has been written to the communication file to unlock the door
					getTime(time);
					logFile = fopen(lockLogFilePath, "a");
					PRINT_MSG(logFile, time, programName, "Wrote command to unlock the door to communication file\n");
					fclose(logFile);     			// Close the log file to finish writing
				}
				else 										// if the previous command stored in the communication file is a 0 (unlocked)
				{
					commFile = fopen(commFilePath, "w");	// Open the communication file
					fprintf(commFile, "1");			// Write a 1 to the communication file indicating that the new command is to lock the door
					fclose(commFile);			// Close the communication file to finish writing

					// Write to the log file that a command has been written to the communication file to lock the door
					getTime(time);
					logFile = fopen(lockLogFilePath, "a");
					PRINT_MSG(logFile, time, programName, "Wrote command to lock the door to communication file\n");
					fclose(logFile);			// Close the log file to finish writing
                		}
                		buttonState = UNPRESSED;
			}
			break;

		case UNPRESSED:

			if (currentButtonValue && !lastButtonValue) 	// button is pressed and was pressed in the previous loop
			{
				buttonState = PRESSED;			// Simply change the buttonState to pressed as commands are only written to the communication file upon button release
			}
			break;
		}
		lastButtonValue = currentButtonValue;			// Assign current button state to previous button state for next iteration

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		currentCommand = readCommunicationFile(commFilePath); 	// Read communication file and assign command to int currentCommand (1 if lock, 0 if unlock)

		switch (lockState)
		{
		case START:

			if (currentCommand)				// If the current command received is 1 or to lock the door
			{
				// Write to the log file that a command to lock the door has been received
				logFile = fopen(lockLogFilePath, "a");
				getTime(time);		
				PRINT_MSG(logFile, time, programName, LOCK_COMMAND_MESSAGE);	
				fclose(logFile);
				
				// Transition to the WAITING TO LOCK state as it is not known yet if the door can be locked
				lockState = WAITING_TO_LOCK;
			}
			else
			{
				// Otherwise write to the log file that a command to unlock the door has been received
				getTime(time);
				logFile = fopen(lockLogFilePath, "a");
				PRINT_MSG(logFile, time, programName, UNLOCK_COMMAND_MESSAGE);

				// Unlock the door before printing a message to the log file that the door has been successfully unlocked
				unlock(gpio);

				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				fclose(logFile);

				// Transition to UNLOCKED state
				lockState = UNLOCKED;
			}
			break;

		case LOCKED:

			if (!currentCommand)		// Continuously check if a command to unlock the door is received from the communication file
			{
				// If a command is received to unlock the door, write to the log file that the command was recieved
				getTime(time);
				logFile = fopen(lockLogFilePath, "a");
				PRINT_MSG(logFile, time, programName, UNLOCK_COMMAND_MESSAGE);

				// Unlock the door before printing a message to the log file that the door has been successfully unlocked
				unlock(gpio);

				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				fclose(logFile);

				// Transition to UNLOCKED state
				lockState = UNLOCKED;
			}
			break;

		case UNLOCKED:

			if (currentCommand)		// Continously check if a command to lock the door is received from the communication file
			{
				// If a command is received to lock the door but the PHOTODIODE is not reading that the laser is hitting it (i.e. reading that the door is not closed)
				if (!readPin(gpio, PHOTODIODE))
				{
					// Print to the log file that the command to lock has been successfully read from the log file, 
					// but the door is open and the program will wait until the door is closed to lock the door
					getTime(time);
					logFile = fopen(lockLogFilePath, "a");
					PRINT_MSG(logFile, time, programName, LOCK_COMMAND_MESSAGE);
					PRINT_MSG(logFile, time, programName, "The door is open, waiting until door is closed to lock \n");
					fclose(logFile);
				}
				// In any case, transition to the WAITING TO LOCK state
				lockState = WAITING_TO_LOCK;
			}
			break;

		case WAITING_TO_LOCK:
			if (!currentCommand) 	// Even in the WAITING TO LOCK state, continously check if a command to unlock the door is received from the communication file
			{
				// If so print a message to the log file that a command has been received to unlock the door
				getTime(time);
				logFile = fopen(lockLogFilePath, "a");
				PRINT_MSG(logFile, time, programName, UNLOCK_COMMAND_MESSAGE);

				// Although the door should not be locked in this state, add a failsafe unlock command to tell the servo to move to that position if it is already not
				// If the servo is already in the unlocked position, this command will not have any bad effects
				unlock(gpio);

				PRINT_MSG(logFile, time, programName, UNLOCKED_MESSAGE);
				fclose(logFile);

				// Transition back to the UNLOCKED state
				lockState = UNLOCKED;
			}

			if (readPin(gpio, PHOTODIODE)) 		// Continously read the photodiode to see if the laser hits the photodiode (i.e. the door has been closed)
			{
				usleep(1000000);		// Sleep for 1 second to prevent the door from jamming on the lock if it is slammed or swung really hard
				lock(gpio);			// Execute the command to lock the door

				// Write to the log file that the door was successfully locked
				getTime(time);
				logFile = fopen(lockLogFilePath, "a");
				PRINT_MSG(logFile, time, programName, LOCKED_MESSAGE);
				fclose(logFile);

				// Transition to the LOCKED state
				lockState = LOCKED;
			}
			break;
		}

		ioctl(watchdog, WDIOC_KEEPALIVE, 0);	// kick the watchdog
		getTime(time);
		logFile = fopen(lockLogFilePath, "a");
		PRINT_MSG(logFile, time, programName, "The Watchdog was updated\n\n");
		fclose(logFile);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	logFile = fopen(lockLogFilePath, "a");	
	
	write(watchdog, "V", 1);	// write to the watchdog to let it know to stop its countdown
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was disabled\n\n");

	close(watchdog);			// close the connection to the watchdog
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was closed\n\n");

	// Clear pins and free GPIO before exiting the program
	cleanup(gpio);				
	gpioTerminate();
	gpiolib_free_gpio(gpio);
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been freed\n\n");

	fclose(logFile);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lock(GPIO_Handle gpio)
{
	clearPin(gpio, GREEN_LED);			// Turn off GREEN LED
	gpioServo(SERVO, LOCKED_FREQUENCY);		// Turn the servo to the locked state
	setPin(gpio, RED_LED);				// Turn on the RED LED to indicate locked state
}

void unlock(GPIO_Handle gpio)
{
	clearPin(gpio, RED_LED);			// Turn off the RED LED
	gpioServo(SERVO, UNLOCKED_FREQUENCY);		// Turn the servo to the unlocked state
	setPin(gpio, GREEN_LED);			// Turn on the GREEN LED to indicate unlocked state
}

void cleanup(GPIO_Handle gpio)
{
	clearPin(gpio, RED_LED);			// Clear RED LED / turn it off
	clearPin(gpio, GREEN_LED);			// Clear GREEN LED / turn it off
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
