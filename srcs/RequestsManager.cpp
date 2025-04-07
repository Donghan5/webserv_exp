#include "RequestsManager.hpp"

RequestsManager::RequestsManager(/* args */)
{
	_config = NULL;
	_client_fd = -1;
}

RequestsManager::RequestsManager(int client_fd) {
	_config = NULL;
	_client_fd = client_fd;
	_partial_requests[_client_fd] = "";
	_partial_responses[_client_fd] = "";
}

RequestsManager::RequestsManager(const RequestsManager &obj) {
	_config = obj._config;
	_client_fd = obj._client_fd;
	_partial_requests[_client_fd] = "";
	_partial_responses[_client_fd] = "";
}

RequestsManager::RequestsManager(HttpConfig *config){
	_config = config;
	_client_fd = -1;
}

RequestsManager::RequestsManager(HttpConfig *config, int client_fd) {
	_config = config;
	_client_fd = client_fd;
	_partial_requests[_client_fd] = "";
	_partial_responses[_client_fd] = "";
}

RequestsManager::~RequestsManager() {

}

void RequestsManager::setConfig(HttpConfig *config) {
	_config = config;
	_partial_requests.erase(_client_fd);
	_partial_responses.erase(_client_fd);
}

void RequestsManager::setClientFd(int client_fd) {
	_client_fd = client_fd;
}

int RequestsManager::HandleRead() {
	Request				request;
	Response			response;
	long long	body_read = -1;

    try {
        char 	buffer[4096];
		int 	nbytes = 1;

		while (nbytes > 0)
		{
			nbytes = read (_client_fd, buffer, 4096);
			if (nbytes <= 0) {
				if (nbytes == 0) {
					std::cerr << "nbytes == 0\n";
					CloseClient();
				}
				throw std::runtime_error("read error");
				return 0;
			}

			if (body_read != -1) {
				// std::cerr << "RequestsManager::HandleRead Reading body: " << body_read << ", + " << nbytes << "\n";
				// std::cerr << "RequestsManager::HandleRead Total size:  " << (_partial_requests[_client_fd].size() + nbytes) << "\n";
				body_read += nbytes;
			}

			if (body_read != -1 && _config->_client_max_body_size && (body_read > _config->_client_max_body_size)) {
				std::cerr << "413 (Request Entity Too Large) error\n";

				_partial_responses[_client_fd] = response.createErrorResponse(413, "text/plain", "Request Entity Too Large", NULL);
				body_read = -1;
				return 2;
			}

			_partial_requests[_client_fd].append(buffer, nbytes);

			int header_end = _partial_requests[_client_fd].find("\r\n\r\n");
			if (body_read == -1 && header_end != CHAR_NOT_FOUND) {
				// std::cerr << "Requests HandleRead: End-of-file Full message:\n" << _partial_requests[_client_fd] << "\n";
				if (!request.setRequest(_partial_requests[_client_fd])) {
					std::cerr << "Requests HandleRead: Error parsing request\n";
					_partial_responses[_client_fd] = response.createErrorResponse(400, "text/plain", "Bad Request", NULL);
					return 2;
				}

				if (request._body_size > 0) {
					std::cerr << "RequestsManager::HandleRead Body needed of size " << request._body_size << "\n";
					body_read = _partial_requests[_client_fd].size() - header_end - 4;
				} else {
					// if (!request.parseBody()) {
					// 	std::cerr << "Requests HandleRead: Error parsing body\n";
					// 	_partial_responses[_client_fd] = response.createErrorResponse(400, "text/plain", "Bad Request", NULL);
					// 	return 2;
					// }
					response.setConfig(_config);
					response.setRequest(&request);

					_partial_responses[_client_fd] = response.getResponse();
					_partial_requests.erase(_client_fd);
					return 2;
				}
			}

			if (body_read != -1 && body_read >= (long long)request._body_size) {
				std::cerr << "RequestsManager::HandleRead Full body read! \n";
				body_read = -1;

				request.clear();
				if (!request.setRequest(_partial_requests[_client_fd])) {
					std::cerr << "Requests HandleRead: Error parsing request\n";
					_partial_responses[_client_fd] = response.createErrorResponse(400, "text/plain", "Bad Request", NULL);
					return 2;
				}
				if (!request.parseBody()) {
					std::cerr << "Requests HandleRead: Error parsing body\n";
					_partial_responses[_client_fd] = response.createErrorResponse(400, "text/plain", "Bad Request", NULL);
					return 2;
				}
				response.setConfig(_config);
				response.setRequest(&request);

				_partial_responses[_client_fd] = response.getResponse();
				_partial_requests.erase(_client_fd);
				return 2;
			}
		}
    } catch (const std::exception& e) {
		body_read = -1;
        std::cerr << "Error in Requests HandleRead: " << e.what() << std::endl;
		_partial_responses[_client_fd] = response.createErrorResponse(500, "text/plain", "Internal Server Error", NULL);
		return 0;
    }
	body_read = -1;
	return 1;
}

int RequestsManager::HandleWrite() {
	std::cerr << "handling write" << "\n";

	try
	{
		STR &response = _partial_responses[_client_fd];
		ssize_t bytes_written = write(_client_fd, response.c_str(), response.length());

		if (bytes_written <= 0) {
			CloseClient();
			return 0;
		}

		response.erase(0, bytes_written);
		_partial_requests.erase(_client_fd);
		if (response.empty()) {
			CloseClient();
			return 0;
		}
		return 1;
	}
	catch(const std::exception& e)
	{
		std::cerr << "HANDLE WRITE error : " << e.what() << '\n';
		return 0;
	}
	return 0;
}

int RequestsManager::HandleClient(short int revents) {
	int status = 0;

	if (_client_fd == -1) {
		return 0;
	}
	if (revents & POLLIN) {
		status = HandleRead();
	}
	if (revents & POLLOUT) {
		HandleWrite();
	}
	if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
		std::cerr << "Requests else\n";
		CloseClient();
		return 0;
	}
	if (status == 2)
		return 2;
	return 1;
}

void RequestsManager::CloseClient() {
	close (_client_fd);
	_partial_requests.erase(_client_fd);
	_partial_responses.erase(_client_fd);
}
