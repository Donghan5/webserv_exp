#ifndef REQUEST_HPP
# define REQUEST_HPP
# include "HttpConfig.hpp"
# include "LocationConfig.hpp"
# include "ServerConfig.hpp"
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
		bool								parseHeader();
		bool								parseBody();
		bool								parseRequest();
		void								parseQueryString();

	public:
		STR									_cookies;
		STR									_full_request;
		STR									_method;
		STR									_file_path;
		STR									_file_name; //based on original path from request or empty if path is a location
		STR									_http_version;
		STR									_host;
		int											_port;
		std::map<STR, float>				_accepted_types; //application/xml;q=0.9
																	//STR      float
		STR									_content_type;
		unsigned long long					_body_size;
		STR									_body;
		STR									_query_string;

		void								setRequest(STR request);

		Request();
		Request(STR request);
		Request(const Request &obj);
		~Request();
};

#endif
