#ifndef SELECTSERVER_HPP
# define SELECTSERVER_HPP
# include "HttpConfig.hpp"
# include "RequestsManager.hpp"

class SelectServer {
	private:
		HttpConfig 					*config;
		bool						running;
		std::map<int, int>			_server_sockets;
		fd_set						_fd_set;
		std::map<int, STR>	_partial_requests;
		std::map<int, STR>	_partial_responses;

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