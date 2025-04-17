#include "PollServer.hpp"
#include "Logger.hpp"

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

static STR intToString(int num) {
	std::ostringstream oss;

	oss << num;

	return oss.str();
}

void PollServer::setConfig(HttpConfig *config) {
	if (!config)
		throw std::runtime_error("Config does not exist");

	this->config = config;

    MAP<int, STR> unique_servers;
    // iterate through the servers and add them to the map. changed to map
    for (size_t i = 0; i < config->_servers.size(); i++) {
		bool is_port_found = false;
        try
		{
			if (unique_servers[(config->_servers[i]->_listen_port)] != "") {
				is_port_found = true;
			}
		}
		catch(const std::exception& e)
		{
			is_port_found = false;
		}

		if (!is_port_found) {
			unique_servers[(config->_servers[i]->_listen_port)] = config->_servers[i]->_listen_server;
		}
    }

	// iterate through the unique servers and create sockets/ switch to map
	for (std::map<int, STR>::iterator it = unique_servers.begin(); it != unique_servers.end(); it++) {
		Logger::cerrlog(Logger::DEBUG, "PollServer::setConfig() - Creating socket for port " + intToString(it->first));
		Logger::cerrlog(Logger::DEBUG, "PollServer::setConfig() - Creating socket for server " + it->second);
		int server_socket = socket(AF_INET, SOCK_STREAM, 0);

		if (server_socket < 0) {
			throw std::runtime_error("Failed to create socket");
		}

		struct timeval timeout;
		timeout.tv_sec = 1;  // 5 seconds
		timeout.tv_usec = 0; // 0 microseconds

		if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to set socket options");
		}

		if (setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to set socket send timeout");
		}

		int reuse = 1;
		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to set reuse address option");
		}

		fcntl(server_socket, F_SETFL, O_NONBLOCK);

		// import part, changing map and using IP address
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(it->first);
		if (it->second == "0.0.0.0") {
			server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			struct addrinfo hints, *result;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_NUMERICHOST;  // Ensure numeric IP parsing

			int status = getaddrinfo(it->second.c_str(), NULL, &hints, &result);
			if (status != 0) {
				// Failed to parse IP
				close(server_socket);
				throw std::runtime_error("Failed to parse IP address");
			}

			// Copy IP address from resolved address
			memcpy(&server_addr.sin_addr,
				   &((struct sockaddr_in*)result->ai_addr)->sin_addr,
				   sizeof(struct in_addr));

			// Free allocated memory
			freeaddrinfo(result);
		}
		// server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
			close(server_socket);
			throw std::runtime_error("Bind error");
		}
		if (listen(server_socket, SOMAXCONN) < 0) {
			close(server_socket);
			throw std::runtime_error("Failed to listen on port");
		}

		// switch map
		_server_sockets[it->first] = server_socket;

		struct pollfd temp_pollfd;
		temp_pollfd.fd = server_socket;
		temp_pollfd.events = POLLIN;
		temp_pollfd.revents = 0;
		_pollfds.push_back(temp_pollfd);

		Logger::log(Logger::INFO, "Server listening on port " + intToString(it->first));
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
			Logger::cerrlog(Logger::DEBUG, "PollServer::CloseClient() - Closing client fd: " + intToString(client_fd));
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
				Logger::cerrlog(Logger::ERROR, "Failed to accept connection");
			return;
		}

		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		struct pollfd client_pollfd;
		client_pollfd.fd = client_fd;
		client_pollfd.events = POLLIN;
		client_pollfd.revents = 0;
		_pollfds.push_back(client_pollfd);
	} catch (const std::exception& e) {
		Logger::cerrlog(Logger::ERROR, "AcceptClient error: " + std::string(e.what()));
	}
}

bool PollServer::WaitAndService(RequestsManager &manager, VECTOR<struct pollfd>	&temp_pollfds) {
	temp_pollfds = _pollfds;

	if (poll(&temp_pollfds[0], temp_pollfds.size(), 5000) < 0) {
        if (errno == EINTR)
            return true;
        throw std::runtime_error("Poll failed");
    }

	for (size_t i = 0; i < temp_pollfds.size(); i++)
	{
		if (temp_pollfds[i].revents == 0)
			continue;

        // Check for errors on all sockets (server and client)
        if (temp_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            Logger::cerrlog(Logger::INFO, "Socket error or hangup for fd: " + intToString(temp_pollfds[i].fd));

            // Check if this is a server socket
            bool is_server_socket = false;
            for (std::map<int, int>::iterator it = _server_sockets.begin(); it != _server_sockets.end(); it++) {
                if (temp_pollfds[i].fd == it->second) {
                    is_server_socket = true;
                    break;
                }
            }

            if (is_server_socket) {
                // This is a serious error on a server socket - might need special handling
                Logger::cerrlog(Logger::ERROR, "Error on server socket: " + intToString(temp_pollfds[i].fd));
                // Consider reopening the server socket or other recovery
            } else {
				// check if this is a cgi process
				bool is_cgi_process = false;
				for (std::map<int, CGIProcess>::iterator it = _cgi_processes.begin(); it != _cgi_processes.end(); it++) {
					if (temp_pollfds[i].fd == it->second.pipefd_out[0]) {
						is_cgi_process = true;

						// close the cgi process
						close(it->second.pipefd_out[0]);
						close(it->second.pipefd_in[1]);

						// search client
						int cliend_fd = -1;
						for (std::map<int, int>::iterator cit = _client_to_cgi.begin(); cit != _client_to_cgi.end(); cit++) {
							if (cit->second == it->first) {
								cliend_fd = cit->first;
								break;
							}
						}

						if (cliend_fd != -1) {
							// send the output of the CGI process to the client
							std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + it->second.partial_output;

							// send the response to the client
							send(cliend_fd, response.c_str(), response.length(), 0);
							_client_to_cgi.erase(cliend_fd);

							// close the client
							CloseClient(cliend_fd);
						}

						_cgi_processes.erase(it->first); // clear CGI process
						break;
					}
				}

                // This is a client socket - just close it (normal client socket)
                CloseClient(temp_pollfds[i].fd);
            }
            continue;
        }

		bool is_server_socket = false;
		for (std::map<int, int>::iterator it = _server_sockets.begin(); it != _server_sockets.end(); it++) {
			if (temp_pollfds[i].fd == it->second) {
				is_server_socket = true;
				break;
			}
		}

		// if (is_server_socket && POLLIN) {
		if (is_server_socket && (temp_pollfds[i].revents & POLLIN)) {
			AcceptClient(temp_pollfds[i].fd);
		} else {
			Logger::cerrlog(Logger::INFO, "New request came in");
			manager.setClientFd(temp_pollfds[i].fd);
			int status = manager.HandleClient(temp_pollfds[i].revents);
			Logger::cerrlog(Logger::DEBUG, "status: " + intToString(status));
			if (!status) {
				for (size_t j = 0; j < _pollfds.size(); ++j) {
					if (_pollfds[j].fd == temp_pollfds[i].fd) {
						_pollfds.erase(_pollfds.begin() + j);
						break;
					}
				}
			} else if (status == 2) {
				for (size_t j = 0; j < _pollfds.size(); ++j) {
					if (_pollfds[j].fd == temp_pollfds[i].fd) {
						_pollfds[j].events = POLLOUT;
						break;
					}
				}
			}
		}
	}
	return true;
}

// excute cgi process
void PollServer::excuteCGI(int client_fd, const std::string &cgi_path, const std::map<std::string, std::string> &env, const std::string& body) {
	int pipefd_in[2], pipefd_out[2];

	if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
		Logger::cerrlog(Logger::ERROR, "Pipe error");
		return;
	}


	// change pipe to non-blocking mode
	fcntl(pipefd_in[0], F_SETFL, O_NONBLOCK);
	fcntl(pipefd_out[0], F_SETFL, O_NONBLOCK);
	fcntl(pipefd_in[1], F_SETFL, O_NONBLOCK);
	fcntl(pipefd_out[1], F_SETFL, O_NONBLOCK);

	// Initialize CGIProcess struct
	CGIProcess process;
	process.pipefd_in[0] = pipefd_in[0];
	process.pipefd_in[1] = pipefd_in[1];
	process.pipefd_out[0] = pipefd_out[0];
	process.pipefd_out[1] = pipefd_out[1];
	process.partial_output = "";


	// fork the process
	pid_t pid = fork();
	if (pid < 0) {
		Logger::cerrlog(Logger::ERROR, "Fork error");

		// close all the pipes
		close(pipefd_in[0]);
		close(pipefd_in[1]);
		close(pipefd_out[0]);
		close(pipefd_out[1]);
		return;
	}

	process.pid = pid;

	if (pid == 0) {  // child process
		dup2(pipefd_out[1], STDOUT_FILENO);
		close(pipefd_out[0]);
		close(pipefd_out[1]);

		dup2(pipefd_in[0], STDIN_FILENO);
		close(pipefd_in[0]);
		close(pipefd_in[1]);

		char **envp = CgiHandler::convertEnvToCharArray(env);
		std::string extension = cgi_path.substr(cgi_path.find_last_of(".") + 1);
		char **args = NULL;

		// set environment variables for CGI
		if (extension == "php") {
			args = CgiHandler::convertArgsToCharArray("/usr/bin/php-cgi", cgi_path);
		}
		else if (extension == "py") {
			args = CgiHandler::convertArgsToCharArray("/usr/bin/python3", cgi_path);
		}
		else if (extension == "pl") {
			args = CgiHandler::convertArgsToCharArray("/usr/bin/perl", cgi_path);
		}
		else if (extension == "sh") {
			args = CgiHandler::convertArgsToCharArray("/bin/bash", cgi_path);
		}
		else {  // unsupported extension
			Logger::cerrlog(Logger::ERROR, "Unsupported CGI extension");
			exit(1);
		}

		// excute CGI
		execve(args[0], args, envp);

		// if execve fails print logs
		Logger::cerrlog(Logger::ERROR, "Execve error" + std::string(strerror(errno)));

		// clean up memories
		for (int i = 0; envp[i] != NULL; i++) {
			free(envp[i]);
		}
		free(envp);

		for (int i = 0; args[i] != NULL; i++) {
			free(args[i]);
		}
		free(args);
		exit(1);
	}
	else {  // parent process
		close(pipefd_out[1]);
		char buffer[4096];
		int byteRead;

		// if body is not empty, write it to the pipe
		if (!body.empty()) {
			write(pipefd_in[1], body.c_str(), body.length());
		}
		close(pipefd_in[0]);

		// set the process in the map to keep track and manage it
		_cgi_processes[pid] = process;
		_client_to_cgi[client_fd] = pid;

		struct pollfd cgi_pollfd;
		cgi_pollfd.fd = pipefd_out[0];
		cgi_pollfd.events = POLLIN;
		cgi_pollfd.revents = 0;
		_pollfds.push_back(cgi_pollfd);
		Logger::log(Logger::INFO, "CGI process created with PID: " + intToString(pid));}
}

void PollServer::start() {
	RequestsManager					manager;
	VECTOR<struct pollfd>	temp_pollfds;

	if (!config) {
		Logger::cerrlog(Logger::ERROR, "Can't start server: config is not set");
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
		Logger::cerrlog(Logger::ERROR, "Can't stop server: config is not set");
	}
	running = false;
}
