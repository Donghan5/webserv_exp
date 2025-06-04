#ifndef CGIUTILS_HPP
#define CGIUTILS_HPP

#include <iostream>
#include "Logger.hpp"
#include "Utils.hpp"

class CgiUtils {
	public:
        static char **convertEnvToCharArray(void);
        static char **convertArgsToCharArray(const std::string &interpreter);
};

#endif