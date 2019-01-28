#include <stdlib.h>
#include <stdio.h>

#define CONFIG_FILE_PATH "lockConfig.cfg"
#define KEY_LOG_FILE_PATH "/home/pi/raspShare/keyLogFile.log"
#define LOCK_LOG_FILE_PATH "/home/pi/locklogFile.log"
#define COMM_FILE_PATH "/home/pi/raspShare/commFile.txt"
#define DEFAULT_LOCK_STATE 0
#define DEFAULT_TIMEOUT 15

void copyProgramName(char* programName, const char* fileName);
int findLength(const char* fileName);
int strCompare(char* compare, char* source);
void strCopy(char* dest, const char* source);
void readConfig(FILE* config, int* timeout, int* lockState, char* commFilePath, char* lockLogFilePath, char* keyLogFilePath);

// MAIN METHOD //
int main(const int argc, const char* const argv[])
{
//////////////////////////////////////////////////////////////////////////// Declare default variables
	int timeout;
	int lockState;

	char keyLogFilePath[255];
	char lockLogFilePath[255];
	char commFilePath[255];

	// Might be editted when reading config, but initialize default values for now
	timeout = DEFAULT_TIMEOUT;
	lockState = DEFAULT_LOCK_STATE;
	
	strCopy(keyLogFilePath, LOCK_LOG_FILE_PATH);
	strCopy(lockLogFilePath, LOCK_LOG_FILE_PATH);
	strCopy(commFilePath, COMM_FILE_PATH);

	int length = findLength(argv[0]);
	char programName[length + 1];
	copyProgramName(programName, argv[0]);	char time[30]; 
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
	FILE *config = fopen(CONFIG_FILE_PATH, "r");
	if (!config)
	{
		perror("The config file could not be opened; using default values");
	}
	else
	{
		readConfig(config, &timeout, &lockState, commFilePath, lockLogFilePath, keyLogFilePath);
	}
	fclose(config);
///////////////////////////////////////////////////////////////////////////////////////////

	printf("WATCHDOG_TIMEOUT = %d %n", timeout);
	printf("DEFAULT_LOCK_STATE = %d %n", lockState);
	printf("COMMMUNICATION_FILE_PATH = %s %n", commFilePath);
	printf("KEY_LOG_FILE_PATH = %s %n", keyLogFilePath);
	printf("LOCK_LOG_FILE_PATH = %s %n", lockLogFilePath);

	return 0;
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

void readConfig(FILE* config, int* timeout, int* lockState, char* commFilePath, char* lockLogFilePath, char* keyLogFilePath)
{
	enum configReadState {START, DONE, COMMENT, WHITESPACE, PARAMETER, TIMEOUT, LOCKSTATE, FILEPATH, AUTOLOCK, AUTOUNLOCK};
	enum configReadState configRead = START;
	
	char buffer[255];

	int i = 0;
	int n = 0;
	char input = buffer[i];
	char parameterName[255];

	while(fgets(buffer, 255, config) != NULL) {
		i = 0;
		n = 0;
		input = buffer[i];
		configRead = START;
		while (input != '\n') {
			input = buffer[i];
			if (input == 0) {
				configRead = DONE;
			}

			switch (configRead)
			{
				case START:
					if (('a' <= input && input <= 'z') || ('A' <= input && input <= 'Z'))
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

				case WHITESPACE:
					if (('a' <= input && input <= 'z') || ('A' <= input && input <= 'Z'))
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
					else if (strCompare("DEFAULT_LOCK_STATE", parameterName))
					{
						configRead = LOCKSTATE;
					}
					else if (strCompare("COMMMUNICATION_FILE_PATH", parameterName)) 
					{
						configRead = FILEPATH;
					}
					else if (strCompare("LOCK_LOG_FILE_PATH", parameterName))
					{
						configRead = FILEPATH;
					}
					else if (strCompare("KEY_LOG_FILE_PATH", parameterName))
					{
						configRead = FILEPATH;
					}
					else
					{
						configRead = COMMENT;
						// perror("Invalid characters in config file.");
						// return;					
					}
					++i;
				break;

				case COMMENT:
					while (input != '\n')
					{
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
				break;

				case TIMEOUT:
				case FILEPATH:
					if (input == '\n' || input == ' ')
					{
						configRead = WHITESPACE;
						break;
					}
					else
					{
						configRead = COMMENT;
						break;
						// perror("Invalid characters in config file.");
						// return;
					}
				break;	

				case DONE:
					return;
				break;
			}

			switch (configRead)
			{
				case COMMENT:
					while (input != '\n')
					{
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
				break;
				
				case WHITESPACE:
					while (input == ' ')
					{
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
				break;

				case PARAMETER:
					
					n = 0;
					input = buffer[i];
					while (input != ' ') 
					{
						if (!(('a' <= input && input <= 'z') || ('A' <= input && input <= 'Z') || ('0' <= input && input <= '9') || input == '_'))
						{
							configRead = COMMENT;
							// perror("Invalid characters in config file.");
							// return;
							break;					
						}
						if (input == 0) {
							configRead = DONE;
							break;
						}
						parameterName[n] = input;
						++n;
						++i;
						input = buffer[i];
					}
					if (configRead != PARAMETER) {
						break;
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
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];	
					}
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
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
					if (configRead == TIMEOUT) {
						*timeout = 0;
					} else {
						break;
					}
					while ('0' <= input && input <= '9') {
						*timeout = (*timeout *10) + (input - '0');
						++i;
						input = buffer[i];
					}
				break;

				case LOCKSTATE:
					input = buffer[i];
					while (!('0' <= input && input <= '1'))
					{
						if (input != ' ')
						{
							configRead = COMMENT;
							// perror("Invalid characters in config file.");
							// return;
							break;
						}
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
					if (configRead != LOCKSTATE) {
						break;
					}

					if (buffer[i] == '1') {
						*lockState = 1;
					} else {
						*lockState = 0;
					}
					++i;
					input = buffer[i];
				break;

				case FILEPATH:
					input = buffer[i];
					
					char* copyPath;
					if (strCompare("COMMMUNICATION_FILE_PATH", parameterName)) 
					{
						copyPath = commFilePath;
					}
					else if (strCompare("LOCK_LOG_FILE_PATH", parameterName))
					{
						copyPath = lockLogFilePath;
					}
					else if (strCompare("KEY_LOG_FILE_PATH", parameterName))
					{
						copyPath = keyLogFilePath;
					}

					else
					{
						break;
					}


					while (input != '/')
					{
						if (input != ' ')
						{
							configRead = COMMENT;
							// perror("Invalid characters in config file.");
							// return;
							break;
						}
						if (input == 0) {
							configRead = DONE;
							break;
						}
						++i;
						input = buffer[i];
					}
					if (configRead != FILEPATH) {
						break;
					}
					
					n = 0;
					while (input != ' ' && input != '\n')
					{
						if (input == 0) {
							configRead = DONE;
							break;
						}
						copyPath[n] = input;
						++n;
						++i;
						input = buffer[i];
					}
					copyPath[n] = 0;
				break;
			}
		}
	}
}