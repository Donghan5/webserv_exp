#include "SelectServer.hpp"

SelectServer::SelectServer(/* args */) {
	config = NULL;
	running = false;
	FD_ZERO(&_fd_set);
}

SelectServer::SelectServer(const SelectServer &obj) {
	this->config = obj.config;
	running = false;
}

SelectServer::SelectServer(HttpConfig *config){
	running = false;
	FD_ZERO(&_fd_set);
	setConfig(config);
}

SelectServer::~SelectServer() {
	// if (config) {
	// 	delete config;
	// }
}

void SelectServer::setConfig(HttpConfig *config) {
	if (!config)
		throw std::runtime_error("Config does not exist");

	this->config = config;

	VECTOR<int>	unique_ports;
	for (size_t i = 0; i < config->_servers.size(); i++) {
		for (size_t j = 0; j < unique_ports.size(); j++) {
			if (unique_ports[j] == config->_servers[i]->_listen_port) {
				continue;
			}
		}
		unique_ports.push_back(config->_servers[i]->_listen_port);
	}

	for (size_t i = 0; i < unique_ports.size(); i++) {
		int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
		if (server_socket < 0) {
			throw std::runtime_error("Failed to create socket");
		}

		// struct timeval timeout;
		// timeout.tv_sec = 5;  // 5 seconds
		// timeout.tv_usec = 0; // 0 microseconds

		// if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		// 	close(server_socket);
		// 	throw std::runtime_error("Failed to set socket options");
		// }

		fcntl(server_socket, F_SETFL, O_NONBLOCK);

		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(unique_ports[i]);
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
		if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
			close(server_socket);
			throw std::runtime_error("Bind error");
		}
		if (listen(server_socket, SOMAXCONN) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to listen on port");
		}

		_server_sockets[unique_ports[i]] = server_socket;

		FD_SET(server_socket, &_fd_set);

    	std::cout << "Server listening on port " << unique_ports[i] << std::endl;
	}

	
}

void SelectServer::HandleWrite(int client_fd) {
	STR &response = _partial_responses[client_fd];
	ssize_t bytes_written = write(client_fd, response.c_str(), response.length());

	if (bytes_written <= 0)
	{
		if (errno != EAGAIN)
		{
			CloseClient(client_fd);
		}
		return;
	}

	response.erase(0, bytes_written);

	if (response.empty())
	{
		CloseClient(client_fd);
	}
}

void SelectServer::HandleRead(int client_fd) {
    try {
        char buffer[4096];
		int nbytes;

		nbytes = read (client_fd, buffer, 4096);
		if (nbytes < 0)
			throw std::runtime_error("read error");
		else if (nbytes == 0)
			/* End-of-file. */
			std::cerr << "HandleRead: End-of-file\n";
		else {
			std::cerr << "HandleRead: Server: got message:" << buffer << "";

		}
    } catch (const std::exception& e) {
        std::cerr << "Error in HandleRead: " << e.what() << std::endl;
        CloseClient(client_fd);
    }
}

void SelectServer::HandleClient(struct pollfd client_poll) {
	if (client_poll.revents & POLLIN)
	{
		std::cerr << "HandleClient handle_client_read\n";
		HandleRead(client_poll.fd);
	}
	if (client_poll.revents & POLLOUT)
	{
		std::cerr << "HandleClient handle_client_write\n";
		HandleWrite(client_poll.fd);
	}
	if (client_poll.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		std::cerr << "HandleClient else\n";
		CloseClient(client_poll.fd);
	}
}

void SelectServer::CloseClient(int client_fd) {
	close(client_fd);
	_partial_requests.erase(client_fd);
	_partial_responses.erase(client_fd);

	FD_CLR(client_fd, &_fd_set);
}

void SelectServer::AcceptClient(int new_fd) {
	try {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(new_fd, (struct sockaddr *)&client_addr, &client_len);

		if (client_fd < 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				std::cerr << "Failed to accept connection" << std::endl;
			return;
		}

		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		FD_SET(client_fd, &_fd_set);
	} catch (const std::exception& e) {
		std::cerr << "AcceptClient Error: " << e.what() << std::endl;
	}
}

bool SelectServer::WaitAndService(RequestsManager &manager, fd_set &temp_fd_set) {
	temp_fd_set = _fd_set;

	if (select (FD_SETSIZE, &temp_fd_set, NULL, NULL, NULL) < 0) {
		if (errno == EINTR)
			return true;
		throw std::runtime_error("Select failed");
	}
	
	for (int i = 0; i < FD_SETSIZE; ++i)
	{
		if (!FD_ISSET (i, &temp_fd_set))
			continue;

		bool is_server_socket = false;
		for (std::map<int, int>::iterator it = _server_sockets.begin(); it != _server_sockets.end(); ++it) {
			if (i == it->second) {
				is_server_socket = true;
				break;
			}
		}

		if (is_server_socket) {
			AcceptClient(i);
		} else {
			manager.setClientFd(i);
			if (!manager.HandleClient()) {
				FD_CLR(i, &_fd_set);
			}
		}
	}

	return true;
}

void SelectServer::start() {
	RequestsManager	manager;
	fd_set		temp_fd_set;

	if (!config) {
		std::cout << "Can't start server: config is not set\n";
	}
	manager.setConfig(config);
	running = true;

	do {
		if (!WaitAndService(manager, temp_fd_set))
			throw std::runtime_error("Poll error");

		// requests.CloseClient(); //??
		
	} while (running);
}

void SelectServer::stop() {
	if (!config) {
		std::cout << "Can't stop server: config is not set\n";
	}
	running = false;
}