#ifndef PI_LOCK
#define PI_LOCK

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

#define GPIO_BASE 0x0
#define GPIO_LEN  0xB4

#define GPFSEL(_x) (_x)
#define GPSET(_x)  (7  + _x)
#define GPCLR(_x)  (10 + _x)
#define GPLEV(_x)  (13 + _x)

#define COMM_FILE_PATH "/home/pi/raspShare/commFile.txt"
#define CONFIG_FILE_PATH "/media/sf_Code/selfdirected/lockConfig.cfg"
#define KEY_LOG_FILE_PATH "/home/pi/raspShare/keyLogFile.log"
#define LOCK_LOG_FILE_PATH "/home/pi/raspShare/locklogFile.log"
#define STAT_FILE_PATH "/home/pi/raspShare/lockStat.stat"
#define DEFAULT_LOCK_STATE 0
#define DEFAULT_TIMEOUT 15

typedef uint32_t* GPIO_Handle;

GPIO_Handle gpiolib_init_gpio(void);
void        gpiolib_free_gpio(GPIO_Handle handle);
void        gpiolib_write_reg(GPIO_Handle handle,uint32_t offst, uint32_t data);
uint32_t    gpiolib_read_reg (GPIO_Handle handle, uint32_t offst);

int selectPin(GPIO_Handle gpio, int pinNumber, int pinType);
int setPin(GPIO_Handle gpio, int pinNumber);
int clearPin(GPIO_Handle gpio, int pinNumber);
int writePin(GPIO_Handle gpio, int pinNumber, int state);
int readPin(GPIO_Handle gpio, int pinNumber);
int freeGPIO(GPIO_Handle gpio);
void getTime(char* buffer);
void copyProgramName(char* programName, const char* fileName);
int findLength(const char* fileName);
int strCompare(char* compare, char* source);
void strCopy(char* dest, const char* source);
void readConfig(FILE* config, int* timeout, int* lockState, char* lockLogFilePath, char* commFilePath, char* statFilePath, char* keyLogFilePath, char* autoLock[], char* autoUnlock[]);

#endif /* PI_LOCK */