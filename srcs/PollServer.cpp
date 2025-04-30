#include "PollServer.hpp"
#include "Logger.hpp"

PollServer::PollServer() : MAX_EVENTS(64) {
    config = NULL;
    running = false;
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to create epoll file descriptor");
    }
    _events.resize(MAX_EVENTS);
}

PollServer::PollServer(const PollServer &obj) : MAX_EVENTS(64) {
    this->config = obj.config;
    running = false;
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to create epoll file descriptor");
    }
    _events.resize(MAX_EVENTS);
}

PollServer::PollServer(HttpConfig *config) : MAX_EVENTS(64) {
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
	 for (MAP<int, STR>::iterator it = unique_servers.begin(); it != unique_servers.end(); ++it) {
        int port = it->first;
        STR server_addr_str = it->second;

        Logger::log(Logger::INFO, "Setting up server on " + server_addr_str + ":" + Utils::intToString(port));

        // Create socket
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            throw std::runtime_error("Failed to create socket: " + STR(strerror(errno)));
        }

        // // For server sockets, keep the timeout but consider using level-triggered mode
        // struct timeval timeout;
        // timeout.tv_sec = 5;  // Increased to 5 seconds for better handling
        // timeout.tv_usec = 0;

        // if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        //     close(server_socket);
        //     throw std::runtime_error("Failed to set receive timeout: " + STR(strerror(errno)));
        // }

        // if (setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        //     close(server_socket);
        //     throw std::runtime_error("Failed to set send timeout: " + STR(strerror(errno)));
        // }

        // Set reuse address option
        int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            close(server_socket);
            throw std::runtime_error("Failed to set reuse address: " + STR(strerror(errno)));
        }

        // Set non-blocking mode
        int flags = fcntl(server_socket, F_GETFL, 0);
        if (flags == -1) {
            close(server_socket);
            throw std::runtime_error("Failed to get socket flags: " + STR(strerror(errno)));
        }

        if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            close(server_socket);
            throw std::runtime_error("Failed to set non-blocking mode: " + STR(strerror(errno)));
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
                                         STR(gai_strerror(status)));
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
                                     Utils::intToString(port) + " - " + STR(strerror(errno)));
        }

        // Listen for connections
        if (listen(server_socket, SOMAXCONN) < 0) {
            close(server_socket);
            throw std::runtime_error("Failed to listen on port " + Utils::intToString(port) +
                                     ": " + STR(strerror(errno)));
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
                        STR(strerror(errno)));
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

	_client_connection_times[client_fd] = std::time(NULL); // Track connection time
	_client_request_received[client_fd] = false; // Track if request header is received

	Logger::cerrlog(Logger::INFO, "New client connection accepted: " + Utils::intToString(client_fd));
}

void PollServer::HandleCgiOutput(int cgi_fd, RequestsManager &manager) {
    // Find the associated client
    MAP<int, int>::iterator it = _cgi_to_client.find(cgi_fd);
    if (it == _cgi_to_client.end()) {
        Logger::cerrlog(Logger::ERROR, "CGI fd without associated client: " + Utils::intToString(cgi_fd));
        RemoveFd(cgi_fd);
        close(cgi_fd);
        return;
    }

    int client_fd = it->second;

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
        Logger::cerrlog(Logger::ERROR, "Error handling CGI output: " + STR(e.what()));

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
    // Use a timeout for epoll_wait to periodically run timeout checks (e.g., 1000ms)
    int num_events = epoll_wait(_epoll_fd, &_events[0], MAX_EVENTS, 1000); // 1 second timeout

    if (num_events < 0) {
        if (errno == EINTR) {
            // Just a signal interruption, not a real error
            // Log in English
            Logger::cerrlog(Logger::DEBUG, "epoll wait interrupted by signal");
            return true; // Continue running
        } else {
            // A real error occurred
             // Log in English
            Logger::cerrlog(Logger::ERROR, "epoll_wait failed: " + STR(strerror(errno)));
            return false; // Stop the server loop on critical error
        }
    }

    // ---> Periodically check for request timeouts <---
    CheckRequestTimeouts(manager); // Call the timeout check function

    // Process ready events
    for (int i = 0; i < num_events; i++) {
        int fd = _events[i].data.fd;
        uint32_t revents = _events[i].events; // Received events

        // Get the fd type (if known)
        FdType fd_type = SERVER_FD; // Default to prevent issues
        MAP<int, FdType>::iterator type_it = _fd_types.find(fd);
        if (type_it != _fd_types.end()) {
            fd_type = type_it->second;
        } else {
            // This fd might have been closed by CheckRequestTimeouts or another event handler
            // Log in English
             Logger::cerrlog(Logger::DEBUG, "Processing event for unknown or already removed fd: " + Utils::intToString(fd));
            continue; // Skip this event
        }

        // Check for errors first (EPOLLERR, EPOLLHUP)
        if (revents & (EPOLLERR | EPOLLHUP)) {
             // Log in English
            Logger::cerrlog(Logger::INFO, "Error or hangup detected on fd " + Utils::intToString(fd) + " (type: " + Utils::intToString(fd_type) + ")");
            if (fd_type == SERVER_FD) {
                 // Log in English
                Logger::cerrlog(Logger::ERROR, "Error on server socket: " + Utils::intToString(fd) + ". Check system logs. Removing from epoll.");
                RemoveFd(fd); // Remove the faulty server socket
                close(fd);    // Close it
            } else if (fd_type == CLIENT_FD) {
                // Client socket error - just close it
                CloseClient(fd);
            } else if (fd_type == CGI_FD) {
                // CGI error or unexpected close
                MAP<int, int>::iterator cgi_it = _cgi_to_client.find(fd);
                if (cgi_it != _cgi_to_client.end()) {
                    int client_fd = cgi_it->second;
                    // Log in English
                    Logger::cerrlog(Logger::INFO, "Error/Hangup on CGI fd " + Utils::intToString(fd) + " (for client " + Utils::intToString(client_fd) + ").");

                    // Even on HUP/ERR, try to process any remaining output
                    manager.setClientFd(client_fd);
                    try {
                        HandleCgiOutput(fd, manager); // This will handle cleanup if CGI finishes or errors out
                    } catch (const std::exception& e) {
                         // Log in English
                        Logger::cerrlog(Logger::ERROR, "Exception handling CGI output on HUP/ERR for fd " + Utils::intToString(fd) + ": " + STR(e.what()));
                        // Ensure cleanup happens if HandleCgiOutput fails
                        RemoveFd(fd);
                        _cgi_to_client.erase(cgi_it);
                        close(fd);
                        // Also close the associated client since CGI failed
                        CloseClient(client_fd);
                    }
                } else {
                    // Orphaned CGI fd error
                     // Log in English
                    Logger::cerrlog(Logger::WARNING, "Error/Hangup on orphaned CGI fd " + Utils::intToString(fd) + ". Removing.");
                    RemoveFd(fd);
                    close(fd);
                }
            }
            continue; // Go to the next event
        }


        // Handle events based on fd type
        if (fd_type == SERVER_FD && (revents & EPOLLIN)) {
            // Server socket has incoming connection(s)
             // Log in English
             Logger::cerrlog(Logger::DEBUG, "EPOLLIN on server fd " + Utils::intToString(fd) + ". Accepting connections.");
            AcceptClient(fd);
        } else if (fd_type == CLIENT_FD) {
            // Client socket has activity
            manager.setClientFd(fd); // Set context for manager
            int status = -1; // Default status

            if (revents & EPOLLIN) {
                // Data available to read
                 // Log in English
                 Logger::cerrlog(Logger::DEBUG, "EPOLLIN on client fd " + Utils::intToString(fd) + ". Calling HandleRead.");
                status = manager.HandleRead();

                // **Crucial:** If HandleRead successfully parsed headers, notify PollServer
                // Assuming HandleRead returns a specific code or modifies state
                // Example check (needs actual implementation in HandleRead):
                // if (manager.checkHeadersReceived(fd)) { // Hypothetical check
                //    _client_request_received[fd] = true;
                // }

            } else if (revents & EPOLLOUT) {
                // Socket ready for writing
                 // Log in English
                 Logger::cerrlog(Logger::DEBUG, "EPOLLOUT on client fd " + Utils::intToString(fd) + ". Calling HandleWrite.");
                status = manager.HandleWrite();
            } else {
                 // Log in English
                 Logger::cerrlog(Logger::WARNING, "Unexpected event combination " + Utils::intToString(revents) + " on client fd " + Utils::intToString(fd));
                 continue; // Skip if neither read nor write ready (shouldn't happen with current flags)
            }


            // Handle status returned by HandleRead/HandleWrite
            if (status == 0) {
                // Remove client
                // Log in English
                Logger::cerrlog(Logger::INFO, "HandleRead/Write returned 0 for client fd " + Utils::intToString(fd) + ". Closing client.");
                CloseClient(fd);
            } else if (status == 1) {
                // Reading: Continue reading (still waiting for data/request complete)
                // Writing: Keep-alive finished writing, close connection now.
                // Need clarification from RequestsManager on status code '1' meaning.
                // Assuming '1' from HandleWrite means close now.
                if (revents & EPOLLOUT) { // Only close if we were writing
                     // Log in English
                     Logger::cerrlog(Logger::INFO, "HandleWrite returned 1 for client fd " + Utils::intToString(fd) + ". Closing client.");
                     CloseClient(fd);
                } else {
                     // Log in English (Reading state)
                     Logger::cerrlog(Logger::DEBUG, "HandleRead returned 1 for client fd " + Utils::intToString(fd) + ". Continuing EPOLLIN.");
                     // If HandleRead returns 1, we likely keep monitoring EPOLLIN (no change needed by ModifyFd)
                }
            } else if (status == 2) {
                // Reading: Request received, need to switch to write response
                // Writing: Still writing data, continue waiting for EPOLLOUT
                if (revents & EPOLLIN) { // If we were reading...
                    // Log in English
                    Logger::cerrlog(Logger::DEBUG, "HandleRead returned 2 for client fd " + Utils::intToString(fd) + ". Switching to EPOLLOUT.");
                    if (!ModifyFd(fd, EPOLLOUT | EPOLLET)) { CloseClient(fd); } // Switch to write mode
                } else { // If we were writing...
                     // Log in English
                     Logger::cerrlog(Logger::DEBUG, "HandleWrite returned 2 for client fd " + Utils::intToString(fd) + ". Continuing EPOLLOUT.");
                     // Remain in write mode (no change needed by ModifyFd)
                }
            } else if (status == 3) {
                // Reading: Not applicable? HandleRead shouldn't return 3?
                // Writing: Finished writing response, switch back to read for Keep-Alive
                if (revents & EPOLLOUT) { // If we were writing...
                     // Log in English
                     Logger::cerrlog(Logger::DEBUG, "HandleWrite returned 3 for client fd " + Utils::intToString(fd) + " (Keep-Alive). Switching back to EPOLLIN.");
                     if (!ModifyFd(fd, EPOLLIN | EPOLLET)) { CloseClient(fd); } // Switch back to read mode
                     // Reset timeout timers for the next request
                     _client_connection_times[fd] = std::time(NULL); // Reset idle timer
                     _client_request_received[fd] = false;         // Reset request received flag
                } else {
                      // Log in English
                      Logger::cerrlog(Logger::WARNING, "HandleRead unexpectedly returned 3 for client fd " + Utils::intToString(fd));
                }
            } else if (status == 4) {
                // Reading: CGI process needs to be started and monitored
                // Writing: Not applicable? HandleWrite shouldn't return 4?
                if (revents & EPOLLIN) { // If we were reading...
                    int cgi_fd = manager.getCurrentCgiFd(); // Get the CGI output fd from manager
                    if (cgi_fd > 0) {
                        // Log in English
                        Logger::cerrlog(Logger::INFO, "HandleRead returned 4 for client fd " + Utils::intToString(fd) + ". Registering CGI fd " + Utils::intToString(cgi_fd) + ".");
                        if (!AddCgiFd(cgi_fd, fd)) { // Register the CGI fd with epoll
                            // Log in English
                            Logger::cerrlog(Logger::ERROR, "Failed to register CGI fd " + Utils::intToString(cgi_fd) + " for client " + Utils::intToString(fd));
                            // Error handling: Send 500 error to client
                             _partial_responses[fd] = manager.createErrorResponse(500, "text/plain", "Failed to start CGI", NULL);
                            if (!ModifyFd(fd, EPOLLOUT | EPOLLET)) { CloseClient(fd); }
                        }
                         // Client fd itself doesn't need modification yet, wait for CGI output
                    } else {
                         // Log in English
                         Logger::cerrlog(Logger::ERROR, "Manager returned invalid CGI fd (" + Utils::intToString(cgi_fd) + ") for client " + Utils::intToString(fd));
                         // Error handling: Send 500 error to client
                         _partial_responses[fd] = manager.createErrorResponse(500, "text/plain", "CGI Internal Error", NULL);
                         if (!ModifyFd(fd, EPOLLOUT | EPOLLET)) { CloseClient(fd); }
                    }
                } else {
                     // Log in English
                     Logger::cerrlog(Logger::WARNING, "HandleWrite unexpectedly returned 4 for client fd " + Utils::intToString(fd));
                }
            }
            // Handle any other potential status codes from manager appropriately

        } else if (fd_type == CGI_FD && (revents & EPOLLIN)) {
            // CGI process has output ready to be read
             // Log in English
             Logger::cerrlog(Logger::DEBUG, "EPOLLIN on CGI fd " + Utils::intToString(fd) + ". Handling CGI output.");
            HandleCgiOutput(fd, manager);
        } else {
             // Log unexpected event/type combination in English
             Logger::cerrlog(Logger::WARNING, "Unhandled event type " + Utils::intToString(fd_type) + " or event flags " + Utils::intToString(revents) + " for fd " + Utils::intToString(fd));
        }
    } // End of event loop

    return true; // Continue server loop
}


void PollServer::CloseClient(int client_fd) {
    // Clean up client resources
    MAP<int, FdType>::iterator type_it = _fd_types.find(client_fd);
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

	_client_connection_times.erase(client_fd);
	_client_request_received.erase(client_fd);

    Logger::cerrlog(Logger::INFO, "Client connection closed: " + Utils::intToString(client_fd));
}

void PollServer::CheckRequestTimeouts(RequestsManager &manager) {
    std::time_t now = std::time(NULL);
    VECTOR<int> timed_out_clients; // Store fds to avoid iterator invalidation

    // Iterate over clients we are tracking connection times for
    for (MAP<int, std::time_t>::iterator it = _client_connection_times.begin(); it != _client_connection_times.end(); ++it) {
        int fd = it->first;

        // Check if we have already received the request for this client
        MAP<int, bool>::iterator received_it = _client_request_received.find(fd);
        if (received_it != _client_request_received.end() && received_it->second) {
            // Request already received, no need to check for request timeout
            // (Idle timeout logic would handle this client later if needed)
            continue;
        }

        // Calculate elapsed time
        double elapsed = difftime(now, it->second);

        if (elapsed > REQUEST_TIMEOUT_SECONDS) {
            Logger::cerrlog(Logger::WARNING, "Client " + Utils::intToString(fd) + " timed out waiting for request header.");
            timed_out_clients.push_back(fd);
        }
    }

    // Process timed-out clients
    for (size_t i = 0; i < timed_out_clients.size(); ++i) {
        int fd = timed_out_clients[i];

         // Ensure the client wasn't closed by another event between checks
        if (_fd_types.find(fd) == _fd_types.end()) {
            continue;
        }

        // Generate 408 response
        // Note: Finding the correct AConfigBase might be tricky here.
        // You might need a default error response or pass NULL.
        manager.setClientFd(fd); // Set context for createErrorResponse if needed
        _partial_responses[fd] = manager.createErrorResponse(408, "text/plain", "Request Timeout", NULL); // Pass NULL for base config

        // Modify epoll to wait for write
        if (!ModifyFd(fd, EPOLLOUT | EPOLLET)) {
             Logger::cerrlog(Logger::ERROR, "Failed to modify client fd " + Utils::intToString(fd) + " for timeout response.");
             CloseClient(fd); // Close immediately if we can't set EPOLLOUT
        } else {
             Logger::cerrlog(Logger::INFO, "Set client " + Utils::intToString(fd) + " to EPOLLOUT for sending 408 response.");
        }

        // Stop tracking connection time for timeout purposes
        _client_connection_times.erase(fd);
        _client_request_received.erase(fd); // Also remove from request status tracking
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
