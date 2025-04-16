#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP
#pragma once

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <map>
#include <cstring>
#include "Utils.hpp"

#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sstream>

class CgiHandler {
	public:
		static char **convertEnvToCharArray(void);
		static char **convertArgsToCharArray(const std::string &interpreter);
		static std::string createErrorResponse(const std::string& status, const std::string& message);

};

#endif
