/* =================================================
 * Authors: Kyle Pinto, Hemit Shah, Efaz Shikder
 * Date: December 03, 2018.
 * UW ID: 20772174, 20756780, 20778157
 * UserIDs: krpinto, h39shah, eashikde
 * -------------------------------------------------
 * HARDWARE CONFIGURATION:
 * 
 * OUTPUTS: GREEN_LED (GPIO 15), RED_LED (GPIO 18)
 * INPUTS: BUTTON (GPIO 14)
 * -------------------------------------------------
 * PROGRAM DESCRIPTION:
 *
 * This code includes functions to read and write 
 * the state of the lock to the commFile.txt file 
 * by means of a simple state machine. It also uses 
 * GPIO functions to read the user input via a button 
 * and output the current state of the lock to the 
 * LED indicators. There are only three states in the
 *  state machine: START,PRESSED and UNPRESSED. When the 
 * state transitions from PRESSED to UNPRESSED, if the 
 * communication file contains the previous command a 
 * command (‘0’) is written to the communication 
 * file to unlock the door, and if the door is 
 * currently unlocked a command (‘1’) is written to
 *  the communication file to lock the door.
 * 
 * ============================================== */

// Import the necessary header files
#include "piLock.h"

// PIN CONSTANTS //
#define BUTTON 14
#define GREEN_LED 15
#define RED_LED 18

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
	FILE *logFile = fopen(keyLogFilePath, "r");
	if (!logFile)
	{
		// If the file does not exist, create a new file at the lockLogFilePath (either default or configuration based)
		// by invoking fopen() with "a" and immediately write a message that a new log file was created
		logFile = fopen(keyLogFilePath, "a");
		getTime(time);
		PRINT_MSG(logFile, time, programName, "# A new log file was created\n\n");
		fclose(logFile);
	}
	fclose(logFile);
	logFile = fopen(keyLogFilePath, "a");
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
	// PIGPIO will handle the servo pin
	getTime(time);
	PRINT_MSG(logFile, time, programName, "Pin 14, 15, 18 have been set to output\n Pin 23 has been set to input\n\n");
	// INTIALIZE OUTPUT PINS //
	clearPin(gpio, GREEN_LED);
	clearPin(gpio, RED_LED);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Close logFile before entering main execution loop to fully write messages to log file
	fclose(logFile);
	

	////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN EXECUTION LOOP //

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
				buttonState = PRESSED;				// Initial case transition to PRESSED if the button pin reads HIGH 
			}
			else
			{
				buttonState = UNPRESSED;			// Else if button is not pressed transition to UNPRESSED
			}
			break;

		case PRESSED:

			if (!currentButtonValue && lastButtonValue) 	// button was pressed in previous loop and has been released -> write a command to the communication file based on the previous state of the
			{
				if (readCommunicationFile(commFilePath))	// if the previous command stored in the communication file is a 1 (locked)
				{
					commFile = fopen(commFilePath, "w");	// Open the communication file
                    fprintf(commFile, "0");					// Write a 0 to the communication file indicating that the new command is to unlock the door
					fclose(commFile);						// Close the communication file to finish writing

					// Write to the log file that a command has been written to the communication file to unlock the door
                    getTime(time);
					logFile = fopen(keyLogFilePath, "a");
                    PRINT_MSG(logFile, time, programName, "Wrote command to unlock the door to communication file\n");
					fclose(logFile);     					// Close the log file to finish writing

					clearPin(gpio, RED_LED);				// Turn off RED_LED
                    setPin(gpio, GREEN_LED);				// Turn on GREEN_LED to indicate message sent to unlock
                }
                else 										// if the previous command stored in the communication file is a 0 (unlocked)
                {
					commFile = fopen(commFilePath, "w");	// Open the communication file
                    fprintf(commFile, "1");					// Write a 1 to the communication file indicating that the new command is to lock the door
					fclose(commFile);						// Close the communication file to finish writing

					// Write to the log file that a command has been written to the communication file to lock the door
                    getTime(time);
					logFile = fopen(keyLogFilePath, "a");
                    PRINT_MSG(logFile, time, programName, "Wrote command to lock the door to communication file\n");
					fclose(logFile);						// Close the log file to finish writing

                    clearPin(gpio, GREEN_LED);				// Turn off GREEN_LED
                    setPin(gpio, RED_LED);					// Turn on RED_LED to indicate message sent to lock
                }
                buttonState = UNPRESSED;
			}
			break;

		case UNPRESSED:

			if (currentButtonValue && !lastButtonValue) 	// button is pressed and was pressed in the previous loop
			{
				buttonState = PRESSED;						// Simply change the buttonState to pressed as commands are only written to the communication file upon button release
			}
			break;
		}
		lastButtonValue = currentButtonValue;				// Assign current button state to previous button state for next iteration


		ioctl(watchdog, WDIOC_KEEPALIVE, 0);	// kick the watchdog and log that the watchdog was updated
		getTime(time);
		logFile = fopen(keyLogFilePath, "a");
		PRINT_MSG(logFile, time, programName, "The Watchdog was updated\n\n");
		fclose(logFile);
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	logFile = fopen(keyLogFilePath, "a");	
	
	write(watchdog, "V", 1);	// write to the watchdog to let it know to stop its countdown
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was disabled\n\n");

	close(watchdog);			// close the connection to the watchdog
	getTime(time);
	PRINT_MSG(logFile, time, programName, "The Watchdog was closed\n\n");

	// Clear pins and free GPIO before exiting the program
	clearPin(gpio, RED_LED);
	clearPin(gpio, GREEN_LED);
	gpioTerminate();
	gpiolib_free_gpio(gpio);
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been freed\n\n");

	fclose(logFile);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}