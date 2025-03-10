#include "PollServer.hpp"

PollServer::PollServer(/* args */) {
	config = NULL;
	running = false;
}

PollServer::PollServer(const PollServer &obj) {
	this->config = obj.config;
	running = false;
}

PollServer::PollServer(HttpConfig *config){
	running = false;
	setConfig(config);
}

PollServer::~PollServer() {
	// if (config) {
	// 	delete config;
	// }
}

void PollServer::setConfig(HttpConfig *config) {
	if (!config)
		throw std::runtime_error("Config does not exist");

	this->config = config;

	std::vector<int>	unique_ports;
	for (size_t i = 0; i < config->_servers.size(); i++) {
		for (size_t j = 0; j < unique_ports.size(); j++) {
			if (unique_ports[j] != config->_servers[i]._listen_port) {
				unique_ports.push_back(config->_servers[i]._listen_port);
			}
		}
		if (unique_ports.empty())
			unique_ports.push_back(config->_servers[i]._listen_port);
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

		int empty;
		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &empty, sizeof(empty)) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to set socket options");
		}

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

		struct pollfd temp_pollfd;
		temp_pollfd.fd = server_socket;
		temp_pollfd.events = POLLIN;
		temp_pollfd.revents = 0;
		_pollfds.push_back(temp_pollfd);

    	std::cout << "Server listening on port " << unique_ports[i] << std::endl;
	}
}

void PollServer::CloseClient(int client_fd) {
	close(client_fd);
	_partial_requests.erase(client_fd);
	_partial_responses.erase(client_fd);

	// Remove from poll_fds
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == client_fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}
}

void PollServer::AcceptClient(int new_fd) {
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

		struct pollfd client_pollfd;
		client_pollfd.fd = client_fd;
		client_pollfd.events = POLLIN;
		client_pollfd.revents = 0;
		_pollfds.push_back(client_pollfd);
	} catch (const std::exception& e) {
		std::cerr << "AcceptClient Error: " << e.what() << std::endl;
	}
}

bool PollServer::WaitAndService(RequestsManager &manager, std::vector<struct pollfd>	&temp_pollfds) {
	temp_pollfds = _pollfds;

	if (poll(&_pollfds[0], _pollfds.size(), -1) < 0) {
		if (errno == EINTR)
			return true;
		throw std::runtime_error("Poll failed");
	}

	for (size_t i = 0; i < temp_pollfds.size(); i++)
	{
		if (temp_pollfds[i].revents == 0)
			continue;

		bool is_server_socket = false;
		for (std::map<int, int>::iterator it = _server_sockets.begin(); it != _server_sockets.end(); it++) {
			if (temp_pollfds[i].fd == it->second) {
				is_server_socket = true;
				break;
			}
		}

		if (is_server_socket && POLLIN) {
			AcceptClient(temp_pollfds[i].fd);
		} else {
			manager.setClientFd(temp_pollfds[i].fd);
			int status = manager.HandleClient();
			if (!status) {
				for (size_t j = 0; j < _pollfds.size(); ++j)
				{
					if (_pollfds[j].fd == temp_pollfds[i].fd)
					{
						_pollfds.erase(_pollfds.begin() + j);
						break;
					}
				}
			} else if (status == 2) {
				for (size_t i = 0; i < _pollfds.size(); ++i) {
					if (_pollfds[i].fd == temp_pollfds[i].fd) {
						_pollfds[i].events = POLLOUT;
						break;
					}
				}
			}
		}
	}
	return true;
}

void PollServer::start() {
	RequestsManager					manager;
	std::vector<struct pollfd>	temp_pollfds;

	if (!config) {
		std::cout << "Can't start server: config is not set\n";
	}
	manager.setConfig(config);
	running = true;

	do {
		if (!WaitAndService(manager, temp_pollfds))
			throw std::runtime_error("Poll error");

		// requests.CloseClient(); //??
		
	} while (running);
}

void PollServer::stop() {
	if (!config) {
		std::cout << "Can't stop server: config is not set\n";
	}
	running = false;
}