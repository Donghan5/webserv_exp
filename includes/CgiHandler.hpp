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
	private:
		std::string _scriptPath;
		std::map<std::string, std::string> _env;
		std::string _body;
		std::map<std::string, std::string> _interpreters;

		char **convertEnvToCharArray(void);
		char **convertArgsToCharArray(const std::string &interpreter);
		std::string createErrorResponse(const std::string& status, const std::string& message);

	public:
		CgiHandler(const std::string &scriptPath, const std::map<std::string, std::string> &env, const std::string &body);
		~CgiHandler();

		std::string executeCgi();
		std::string executeProxy();
};

#endif
