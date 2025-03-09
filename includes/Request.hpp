#ifndef REQUEST_HPP
# define REQUEST_HPP
# include "HttpConfig.hpp"
# include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <map>

class Request {
	private:
		std::string	_file_path;
		std::string	_mime_type;

	public:
		Request();
		Request(const Request &obj);
		~Request();
		
};

#endif