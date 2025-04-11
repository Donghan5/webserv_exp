#include "../includes/Logger.hpp"


// get current time
std::string Logger::getCurrentTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];

	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

	return std::string(buf);
}

// convert log level to string
std::string Logger::logLevelToString(LogLevel level) {
	switch(level) {
		case INFO: return "INFO";
		case WARNING: return "WARNING";
		case ERROR: return "ERROR";
		case DEBUG: return "DEBUG";
		default: return "UNKNOWN";
	}
}

// Just print the log cout
void Logger::log(LogLevel level, const std::string &message) {

	std::string logEntry = "[" + Logger::getCurrentTime() + "] " + "[" + Logger::logLevelToString(level) + "] " + ": " + message;

	std::cout << logEntry << std::endl;
}

// separate error log using cerr
void Logger::cerrlog(LogLevel level, const std::string &message) {
	std::string logEntry = "[" + Logger::getCurrentTime() + "] " + "[" + Logger::logLevelToString(level) + "] " + ": " + message;

	std::cerr << logEntry << std::endl;
}
