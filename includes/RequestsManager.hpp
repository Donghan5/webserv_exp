#ifndef REQUESTSMANAGER_HPP
# define REQUESTSMANAGER_HPP
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

class RequestsManager {
	private:
		HttpConfig 					*_config;
		int							_client_fd;
		std::map<int, std::string>	_partial_requests;
		std::map<int, std::string>	_partial_responses;

		bool						HandleRead();
		bool 						HandleWrite();

	public:
		RequestsManager();
		RequestsManager(int client_fd);
		RequestsManager(HttpConfig *config);
		RequestsManager(HttpConfig *config, int client_fd);
		RequestsManager(const RequestsManager &obj);
		~RequestsManager();
		
		void setConfig(HttpConfig *config);
		void setClientFd(int client_fd);
		bool HandleClient();
		void CloseClient();
};

#endif