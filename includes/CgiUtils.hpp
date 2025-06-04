#ifndef CGIUTILS_HPP
#define CGIUTILS_HPP

#include <iostream>
#include "Logger.hpp"
#include "Utils.hpp"
#include <string>
#include <map>
#include <cstring>
#include <unistd.h>

class CgiUtils {
	public:
        static char **convertEnvToCharArray(std::map<std::string, std::string> &env);
        static char **convertArgsToCharArray(const std::string &interpreter, const std::string &scriptPath);
		static void closePipes(int input_pipe0, int input_pipe1, int output_pipe0, int output_pipe1);
        static std::string createErrorResponseCgi(const std::string& status, const std::string& message);

};

#endif
