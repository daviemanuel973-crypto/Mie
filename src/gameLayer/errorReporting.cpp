#include <errorReporting.h>
#include <fstream>
#include <iostream>

void createErrorFile()
{
	std::fstream f(USER_ERROR_LOG_PATH);
	f.close();
}



void reportError(const char *message)
{
	//std::fstream f(USER_ERROR_LOG_PATH, std::ios::app);
	//f << message << "\n";
	//f.close();
	std::cout << message << "\n";
}