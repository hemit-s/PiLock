#ifndef PI_LOCK
#define PI_LOCK

/* =================================================
 * Authors: Kyle Pinto, Hemit Shah, Efaz Shikder
 * Date: December 03, 2018.
 * UW ID: 20772174, 20756780, 20778157
 * UserIDs: krpinto, h39shah, eashikde
 * 
 * -------------------------------------------------
 * PROGRAM DESCRIPTION:
 *
 * This is a header file file which declares the functions
 * that are necessary for Lock.c and Key.c. Implementions 
 * can be found in piLock.c.  This header also contains 
 * defined constants for GPIO hardware registers, file 
 * location paths, and default states for the system.  
 * Finally, the header is a culminative structure that 
 * includes all the libraries that bot key.c and lock.c 
 * are dependednt on, allowing for a single header to 
 * be inlcuded in each file.  Not that the PIGPIO library 
 * is not included here as only lock.c requires it.  
 * Thus, it is inlcuded seperately in lock.c so as to 
 * avoid inporting unescessary libraries in key.c.  This 
 * means that this header inlcudes libraries that are common
 * to both key.c and lock.c
 * ============================================== */

// Import necessary libraries
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <stdint.h>

#include <linux/watchdog.h> // watchdog specific constants
#include <sys/ioctl.h> // ioctl

#include <time.h> // time_t and time()
#include <sys/time.h> // getTimeOfDay()

// Define default GPIO variables
#define GPIO_BASE 0x0
#define GPIO_LEN  0xB4
// Define GPIO library macros
#define GPFSEL(_x) (_x)
#define GPSET(_x)  (7  + _x)
#define GPCLR(_x)  (10 + _x)
#define GPLEV(_x)  (13 + _x)

// Define macro used to pring logging messages to the log file
#define PRINT_MSG(file, time, programName, outputStr)                   \
	do                                                                  \
	{                                                                   \
		fprintf(logFile, "%s : %s : %s", time, programName, outputStr); \
	} while (0)

// Define the default file paths and default variable values for values stored in the configuration file
#define COMM_FILE_PATH "/home/pi/raspShare/commFile.txt"
#define CONFIG_FILE_PATH "/home/pi/raspShare/lockConfig.cfg"
#define KEY_LOG_FILE_PATH "/home/pi/raspShare/keyLogFile.log"
#define LOCK_LOG_FILE_PATH "/home/pi/raspShare/locklogFile.log"
#define DEFAULT_LOCK_STATE 0
#define DEFAULT_TIMEOUT 15

typedef uint32_t* GPIO_Handle;
// Imported GPIO Functions
GPIO_Handle gpiolib_init_gpio(void);
void        gpiolib_free_gpio(GPIO_Handle handle);
void        gpiolib_write_reg(GPIO_Handle handle,uint32_t offst, uint32_t data);
uint32_t    gpiolib_read_reg (GPIO_Handle handle, uint32_t offst);

// Self created GPIO Functions
int selectPin(GPIO_Handle gpio, int pinNumber, int pinType);
int setPin(GPIO_Handle gpio, int pinNumber);
int clearPin(GPIO_Handle gpio, int pinNumber);
int writePin(GPIO_Handle gpio, int pinNumber, int state);
int readPin(GPIO_Handle gpio, int pinNumber);
int freeGPIO(GPIO_Handle gpio);

// Logging specific functions used to determine the time and program name
void getTime(char* buffer);
int findLength(const char* fileName);
void copyProgramName(char* programName, const char* fileName);

// Config file reading specific functions used to compare and store parameter names while parsing the data
int strCompare(const char* compare, const char* source);
void strCopy(char* dest, const char* source);
void readConfig(FILE* config, int* timeout, int* lockState, char* commFilePath, char* lockLogFilePath, char* keyLogFilePath);

// Communication file reading specific funciton
int readCommunicationFile(char *commFilePath);

#endif /* PI_LOCK */