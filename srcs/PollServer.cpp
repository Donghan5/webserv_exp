#include "PollServer.hpp"
#include "Logger.hpp"

PollServer::PollServer() {
    config = NULL;
    running = false;
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to create epoll file descriptor");
    }
    _events.resize(MAX_EVENTS);
}

PollServer::PollServer(const PollServer &obj) {
    this->config = obj.config;
    running = false;
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to create epoll file descriptor");
    }
    _events.resize(MAX_EVENTS);
}

PollServer::PollServer(HttpConfig *config){
    running = false;
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to create epoll file descriptor");
    }
    _events.resize(MAX_EVENTS);
    setConfig(config);
}

PollServer::~PollServer() {
    if (_epoll_fd >= 0) {
        close(_epoll_fd);
    }
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

	 // Create sockets for unique server entries
	 for (std::map<int, STR>::iterator it = unique_servers.begin(); it != unique_servers.end(); ++it) {
        int port = it->first;
        STR server_addr_str = it->second;

        Logger::log(Logger::INFO, "Setting up server on " + server_addr_str + ":" + Utils::intToString(port));

        // Create socket
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        // // For server sockets, keep the timeout but consider using level-triggered mode
        // struct timeval timeout;
        // timeout.tv_sec = 5;  // Increased to 5 seconds for better handling
        // timeout.tv_usec = 0;

        // if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        //     close(server_socket);
        //     throw std::runtime_error("Failed to set receive timeout: " + std::string(strerror(errno)));
        // }

        // if (setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        //     close(server_socket);
        //     throw std::runtime_error("Failed to set send timeout: " + std::string(strerror(errno)));
        // }

        // Set reuse address option
        int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            close(server_socket);
            throw std::runtime_error("Failed to set reuse address: " + std::string(strerror(errno)));
        }

        // Set non-blocking mode
        int flags = fcntl(server_socket, F_GETFL, 0);
        if (flags == -1) {
            close(server_socket);
            throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
        }

        if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            close(server_socket);
            throw std::runtime_error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
        }

        // Set up server address
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (server_addr_str == "0.0.0.0") {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_NUMERICHOST;

            int status = getaddrinfo(server_addr_str.c_str(), NULL, &hints, &result);
            if (status != 0) {
                close(server_socket);
                throw std::runtime_error("Failed to parse IP address: " +
                                         std::string(gai_strerror(status)));
            }

            memcpy(&addr.sin_addr,
                   &((struct sockaddr_in*)result->ai_addr)->sin_addr,
                   sizeof(struct in_addr));

            freeaddrinfo(result);
        }

        // Bind socket
        if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_socket);
            throw std::runtime_error("Failed to bind to " + server_addr_str + ":" +
                                     Utils::intToString(port) + " - " + std::string(strerror(errno)));
        }

        // Listen for connections
        if (listen(server_socket, SOMAXCONN) < 0) {
            close(server_socket);
            throw std::runtime_error("Failed to listen on port " + Utils::intToString(port) +
                                     ": " + std::string(strerror(errno)));
        }

        // Store socket in map
        _server_sockets[port] = server_socket;

        // Add to epoll monitoring - consider using level-triggered mode (removing EPOLLET)
        // for better handling of large requests
		if (!AddFd(server_socket, EPOLLIN, SERVER_FD)) {
			close(server_socket);
			throw std::runtime_error("Failed to add server socket to epoll");
		}

        Logger::log(Logger::INFO, "Server listening on " + server_addr_str + ":" + Utils::intToString(port));
    }
}

 // Add file descriptor to epoll monitoring
 bool PollServer::AddFd(int fd, uint32_t events, FdType type) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
		Logger::cerrlog(Logger::ERROR, "Failed to add fd to epoll");
		return false;
	}

    // Store the fd type
    _fd_types[fd] = type;

	return true;
}

bool PollServer::AddCgiFd(int cgi_fd, int client_fd) {
    if (AddFd(cgi_fd, EPOLLIN, CGI_FD)) {
        _cgi_to_client[cgi_fd] = client_fd;
        return true;
    }
    return false;
}

// Modify epoll events for a file descriptor
bool PollServer::ModifyFd(int fd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0) {
		Logger::cerrlog(Logger::ERROR, "Failed to modify fd in epoll");
		return false;
	}
	return true;
}

// Fix the RemoveFd method to handle the case where the fd might have already been removed
bool PollServer::RemoveFd(int fd) {
    // Check if the fd is actually in our tracking map
    if (_fd_types.find(fd) == _fd_types.end()) {
        Logger::cerrlog(Logger::DEBUG, "RemoveFd: File descriptor " + Utils::intToString(fd) + " not found in tracking map");
        return true; // Return success if the fd wasn't in our map
    }

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        if (errno == EBADF || errno == ENOENT) {
            // The fd is invalid or was already removed, just clean up our tracking
            Logger::cerrlog(Logger::DEBUG, "RemoveFd: File descriptor " + Utils::intToString(fd) +
                        " was already closed or removed from epoll");
        } else {
            Logger::cerrlog(Logger::ERROR, "Failed to remove fd from epoll: " +
                        std::string(strerror(errno)));
            return false;
        }
    }

    // Remove from fd type tracking
    _fd_types.erase(fd);

    return true;
}

// Add server socket
bool PollServer::AddServerSocket(int port, int socket_fd) {
    _server_sockets[port] = socket_fd;
    return AddFd(socket_fd, EPOLLIN, SERVER_FD);
}

// Accept new client connection
void PollServer::AcceptClient(int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		Logger::cerrlog(Logger::ERROR, "Failed to accept client connection");
		return;
	}

	// Set non-blocking
	int flags = fcntl(client_fd, F_GETFL, 0);
	fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

	// Add to epoll for read events
	if (!AddFd(client_fd, EPOLLIN | EPOLLET, CLIENT_FD)) {
		Logger::cerrlog(Logger::ERROR, "Failed to add client fd to epoll");
		close(client_fd);
		return;
	}

	Logger::cerrlog(Logger::INFO, "New client connection accepted: " + Utils::intToString(client_fd));
}

void PollServer::HandleCgiOutput(int cgi_fd, RequestsManager &manager) {
    // Find the associated client
    std::map<int, int>::iterator it = _cgi_to_client.find(cgi_fd);
    if (it == _cgi_to_client.end()) {
        Logger::cerrlog(Logger::ERROR, "CGI fd without associated client: " + Utils::intToString(cgi_fd));
        RemoveFd(cgi_fd);
        close(cgi_fd);
        return;
    }

    int client_fd = it->second;
    Logger::cerrlog(Logger::INFO, "Processing CGI output for client: " + Utils::intToString(client_fd));

    try {
        // Process the CGI output
        int result = manager.HandleCgiOutput(cgi_fd);

        if (result > 0) {
            // CGI completed, switch client to write mode
            if (!ModifyFd(client_fd, EPOLLOUT | EPOLLET)) {
                Logger::cerrlog(Logger::ERROR, "Failed to modify client fd for writing");
                CloseClient(client_fd);
            }

            // Remove the CGI fd from epoll and tracking
            RemoveFd(cgi_fd);
            _cgi_to_client.erase(it);

            // Don't close the fd here, the Response object owns it
        } else if (result == 0) {
            // Error occurred, clean up
            RemoveFd(cgi_fd);
            _cgi_to_client.erase(it);
            close(cgi_fd);

            // Close the client as well
            CloseClient(client_fd);
        }
        // result < 0 means CGI still running, keep monitoring
    } catch (const std::exception& e) {
        Logger::cerrlog(Logger::ERROR, "Error handling CGI output: " + std::string(e.what()));

        // Clean up
        RemoveFd(cgi_fd);
        _cgi_to_client.erase(it);
        close(cgi_fd);

        // Close the client as well
        CloseClient(client_fd);
    }
}

// Fix the WaitAndService method to properly handle CGI events
bool PollServer::WaitAndService(RequestsManager &manager) {
    int num_events = epoll_wait(_epoll_fd, &_events[0], MAX_EVENTS, -1);

    if (num_events < 0) {
        if (errno == EINTR) {
            // Just a signal interruption, not a real error
            Logger::cerrlog(Logger::DEBUG, "Epoll wait was interrupted by a signal");
            return true;
        } else {
            // A real error occurred
            Logger::cerrlog(Logger::ERROR, "Epoll wait failed: " + std::string(strerror(errno)));
            return false;
        }
    }

    for (int i = 0; i < num_events; i++) {
        int fd = _events[i].data.fd;

        // Get the fd type (if known)
        FdType fd_type = SERVER_FD; // Default to prevent issues
        if (_fd_types.find(fd) != _fd_types.end()) {
            fd_type = _fd_types[fd];
        } else {
            Logger::cerrlog(Logger::WARNING, "Unknown fd type for fd: " + Utils::intToString(fd));
            continue; // Skip this fd if we don't know what it is
        }

        // Check for errors first
        if (_events[i].events & (EPOLLERR | EPOLLHUP)) {
            Logger::cerrlog(Logger::INFO, "Socket error or hangup for fd: " + Utils::intToString(fd));

            if (fd_type == SERVER_FD) {
                Logger::cerrlog(Logger::ERROR, "Error on server socket: " + Utils::intToString(fd));
                // Handle server socket error - possibly try to reopen
            } else if (fd_type == CLIENT_FD) {
                // This is a client socket - just close it
                CloseClient(fd);
            } else if (fd_type == CGI_FD) {
                // CGI error or completion
                std::map<int, int>::iterator it = _cgi_to_client.find(fd);
                if (it != _cgi_to_client.end()) {
                    int client_fd = it->second;

                    // Note: Even a hangup might have produced output we need to read
                    manager.setClientFd(client_fd);

                    try {
                        // Try to read any available data before closing
                        HandleCgiOutput(fd, manager);
                    } catch (const std::exception& e) {
                        Logger::cerrlog(Logger::ERROR, "Error handling CGI output on hangup: " + std::string(e.what()));
                        // Clean up CGI resources
                        RemoveFd(fd);
                        _cgi_to_client.erase(it);
                        close(fd);
                        // Create an error response and set client to write mode
                        _partial_responses[client_fd] = "HTTP/1.1 500 Internal Server Error\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 9\r\n"
                               "\r\n"
                               "CGI Error";
                        ModifyFd(client_fd, EPOLLOUT | EPOLLET);
                    }
                } else if (fd_type == POST_FD && (_events[i].events & EPOLLOUT)) {
					// POST 쓰기 준비된 파일 디스크립터
					HandlePostWrite(fd, manager);
				} else {
                    // Orphaned CGI fd - just remove it
                    RemoveFd(fd);
                    close(fd);
                }
            }
            continue;
        }

        // Handle events based on fd type
        if (fd_type == SERVER_FD && (_events[i].events & (EPOLLIN | EPOLLOUT))) {
            // Server socket has incoming connection
            AcceptClient(fd);
        } else if (fd_type == CLIENT_FD) {
            // Client socket has activity
            Logger::cerrlog(Logger::INFO, "Event for client fd: " + Utils::intToString(fd));
            manager.setClientFd(fd);
            int status = manager.HandleClient(_events[i].events);

            if (status == 0) {
                // Remove client
                CloseClient(fd);
            } else if (status == 2) {
                // Switch to monitoring for write events
                ModifyFd(fd, EPOLLOUT | EPOLLET);
            } else if (status == 3) {
                // Switch back to read events (for persistent connections)
                ModifyFd(fd, EPOLLIN | EPOLLET);
            } else if (status == 4) {
                // Special case: register CGI fd
                int cgi_fd = manager.getCurrentCgiFd();
                if (cgi_fd > 0) {
                    AddCgiFd(cgi_fd, fd);
                } else {
                    Logger::cerrlog(Logger::ERROR, "Invalid CGI file descriptor returned from manager");
                    // Switch back to client handling in error state
                    ModifyFd(fd, EPOLLOUT | EPOLLET);
                    manager.setClientFd(fd);
                }
            } else if (status == 5) {
				// POST 작업 등록
				int post_fd = manager.getCurrentPostFd();
				if (post_fd > 0) {
					AddPostFd(post_fd, fd);
				}
				else {
					Logger::cerrlog(Logger::ERROR, "Invalid POST file descriptor returned from manager");
					// 오류 상태로 클라이언트 전환
					ModifyFd(fd, EPOLLOUT | EPOLLET);
				}
			}
        } else if (fd_type == CGI_FD && (_events[i].events & EPOLLIN)) {
            // CGI output ready
            HandleCgiOutput(fd, manager);
        }
    }
    return true;
}

void PollServer::CloseClient(int client_fd) {
    // Clean up client resources
    std::map<int, FdType>::iterator type_it = _fd_types.find(client_fd);
    if (type_it != _fd_types.end()) {
        _fd_types.erase(type_it);
    }

    // Remove from epoll
	RemoveFd(client_fd);

    // Close socket
	close(client_fd);

    // Clear request/response data
	_partial_requests.erase(client_fd);
	_partial_responses.erase(client_fd);

    Logger::cerrlog(Logger::INFO, "Client connection closed: " + Utils::intToString(client_fd));
}

bool PollServer::AddPostFd(int post_fd, int client_fd) {
    if (AddFd(post_fd, EPOLLOUT, POST_FD)) {
        _post_to_client[post_fd] = client_fd;
        return true;
    }
    return false;
}

// POST 쓰기 작업 처리 메소드
void PollServer::HandlePostWrite(int post_fd, RequestsManager &manager) {
    // 연결된 클라이언트 찾기
    std::map<int, int>::iterator it = _post_to_client.find(post_fd);
    if (it == _post_to_client.end()) {
        Logger::cerrlog(Logger::ERROR, "POST fd without associated client: " + Utils::intToString(post_fd));
        RemoveFd(post_fd);
        close(post_fd);
        return;
    }

    int client_fd = it->second;
    Logger::cerrlog(Logger::INFO, "Processing POST write for client: " + Utils::intToString(client_fd));

    try {
        // POST 쓰기 작업 처리
        manager.setClientFd(client_fd);
        int result = manager.HandlePostWrite(post_fd);

        if (result > 0) {
            // POST 완료, 클라이언트를 쓰기 모드로 전환
            if (!ModifyFd(client_fd, EPOLLOUT | EPOLLET)) {
                Logger::cerrlog(Logger::ERROR, "Failed to modify client fd for writing after POST");
                CloseClient(client_fd);
            }

            // POST fd 제거
            RemoveFd(post_fd);
            _post_to_client.erase(it);
        }
        else if (result == 0) {
            // 오류 발생, 리소스 정리
            RemoveFd(post_fd);
            _post_to_client.erase(it);
            close(post_fd);

            // 클라이언트도 종료
            CloseClient(client_fd);
        }
        // result < 0는 POST가 아직 진행 중이므로 계속 모니터링
    }
    catch (const std::exception& e) {
        Logger::cerrlog(Logger::ERROR, "Error handling POST write: " + std::string(e.what()));

        // 리소스 정리
        RemoveFd(post_fd);
        _post_to_client.erase(it);
        close(post_fd);

        // 클라이언트도 종료
        CloseClient(client_fd);
    }
}

void PollServer::start() {
	RequestsManager					manager;

	if (!config) {
		Logger::cerrlog(Logger::ERROR, "Can't start server: config is not set");
	}
	manager.setConfig(config);
	running = true;

	do {
		if (!WaitAndService(manager))
			throw std::runtime_error("Poll error");
	} while (running);
}

void PollServer::stop() {
	if (!config) {
		Logger::cerrlog(Logger::ERROR, "Can't stop server: config is not set");
	}
	running = false;
}
