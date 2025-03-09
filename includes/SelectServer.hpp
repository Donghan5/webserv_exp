#ifndef SELECTSERVER_HPP
# define SELECTSERVER_HPP
# include "HttpConfig.hpp"
# include "RequestsManager.hpp"
# include <iostream>

//to clean
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

class SelectServer {
	private:
		HttpConfig 					*config;
		bool						running;
		std::map<int, int>			_server_sockets;
		fd_set						_fd_set;
		std::map<int, std::string>	_partial_requests;
		std::map<int, std::string>	_partial_responses;

		bool						WaitAndService(RequestsManager &requests, fd_set &temp_fd_set);
		void						AcceptClient(int new_fd);
		void						HandleClient(struct pollfd client_fd);
		void						HandleRead(int client_fd);
		void 						CloseClient(int client_fd);
		void 						HandleWrite(int client_fd);

	public:
		SelectServer();
		SelectServer(const SelectServer &obj);
		SelectServer(HttpConfig *config);
		~SelectServer();

		void setConfig(HttpConfig *config);

		void start();
		void stop();
};

#endif