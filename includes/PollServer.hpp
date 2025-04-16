#ifndef POLLSERVER_HPP
# define POLLSERVER_HPP
# include "HttpConfig.hpp"
# include "RequestsManager.hpp"
# include <iostream>
# include "CgiHandler.hpp"

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

class PollServer {
	private:
		HttpConfig 					*config;
		bool						running;
		std::map<int, int>			_server_sockets;
		VECTOR<struct pollfd>		_pollfds;
		std::map<int, STR>			_partial_requests;
		std::map<int, STR>			_partial_responses;

		bool						WaitAndService(RequestsManager &requests, VECTOR<struct pollfd>	&temp_pollfds);
		void						AcceptClient(int new_fd);
		void 						CloseClient(int client_fd);
		void						excuteCGI(int client_fd, const std::string &cgi_path, const std::map< std::string, std::string > &env, const std::string& body);

		struct CGIProcess{
			pid_t pid;
			int pipefd_in[2];
			int pipefd_out[2];
			std::string partial_output;
		};

		std::map<int, CGIProcess>	_cgi_processes;
		std::map<int, int> _client_to_cgi;

	public:
		PollServer();
		PollServer(const PollServer &obj);
		PollServer(HttpConfig *config);
		~PollServer();

		void setConfig(HttpConfig *config);

		void start();
		void stop();

};

#endif
