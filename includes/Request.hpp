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

enum ChunkedState {
	CHUNK_SIZE,  // chunk size
	CHUNK_EXT,  // chunk extension
	CHUNK_DATA,  // chunk data
	CHUNK_CR,  // chunk CR
	CHUNK_LF,  // chunk LF
	CHUNK_DATA_END, // chunk data end
	CHUNK_TRAILER,  // chunk trailer
	CHUNK_COMPLETE,  // chunk complete
};

class Request {
	private:
		bool								parseHeader();
		bool								parseBody();
		bool								parseRequest();
		void								parseQueryString();
		void								parseTransferEncoding(const std::string &header);
		std::string							parseDataChunked(const std::string &raw_body);

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
		std::vector<STR>					_transfer_encoding;
		bool								_chunked_flag;
		ChunkedState						_chunked_state;
		unsigned long long					_chunk_size;
		unsigned long long					_chunk_data_read;

		void								setRequest(STR request);

		Request();
		Request(STR request);
		Request(const Request &obj);
		~Request();
};

#endif
