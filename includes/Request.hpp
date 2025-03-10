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
		bool										parseRequest();
	
	public:
		std::string									_full_request;
		std::string									_method;
		std::string									_file_path;
		std::string									_http_version;
		std::string									_host;
		int											_port;
		std::map<std::string, float>				_accepted_types; //application/xml;q=0.9
																	//std::string      float
		std::string									_content_type;
		
		void										setRequest(std::string request);
		
		Request();
		Request(std::string request);
		Request(const Request &obj);
		~Request();
};

#endif