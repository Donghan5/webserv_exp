#ifndef POLLSERVER_HPP
# define POLLSERVER_HPP
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
#include <fcntl.h>
#include <map>
#include <sys/epoll.h>


enum FdType {
    SERVER_FD,
    CLIENT_FD,
    CGI_FD,
    POST_FD
};



class PollServer {
	private:
	HttpConfig 					*config;
	bool						running;
	std::map<int, int>			_server_sockets;      // port -> socket_fd
	std::map<int, STR>			_partial_requests;
	std::map<int, STR>			_partial_responses;
	std::map<int, FdType>       _fd_types;           // Track fd types
	std::map<int, int>          _cgi_to_client;      // Map CGI fd to client fd
	int							_epoll_fd;
	VECTOR<struct epoll_event>	_events;
	const int MAX_EVENTS = 64;

	bool						WaitAndService(RequestsManager &requests);
	void						AcceptClient(int new_fd);
	void 						CloseClient(int client_fd);
	void                        HandleCgiOutput(int cgi_fd, RequestsManager &requests);
	bool AddFd(int fd, uint32_t events, FdType type);
	bool ModifyFd(int fd, uint32_t events);
	bool RemoveFd(int fd);
	bool AddServerSocket(int port, int socket_fd);
	bool AddCgiFd(int cgi_fd, int client_fd);
	// PollServer 클래스에 추가할 맵:
	std::map<int, int> _post_to_client;      // POST 파일 fd에서 클라이언트 fd로의 매핑

	// PollServer 클래스에 추가할 메소드:
	bool AddPostFd(int post_fd, int client_fd);
	void HandlePostWrite(int post_fd, RequestsManager &manager);

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
