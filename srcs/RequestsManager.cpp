#include "RequestsManager.hpp"

RequestsManager::RequestsManager(/* args */)
{
	_config = NULL;
	_client_fd = -1;
}

RequestsManager::RequestsManager(int client_fd) {
	_config = NULL;
	_client_fd = client_fd;
}

RequestsManager::RequestsManager(const RequestsManager &obj) {
	_config = obj._config;
	_client_fd = obj._client_fd;
}

RequestsManager::RequestsManager(HttpConfig *config){
	_config = config;
	_client_fd = -1;
}

RequestsManager::RequestsManager(HttpConfig *config, int client_fd) {
	_config = config;
	_client_fd = client_fd;
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
	_partial_requests.erase(_client_fd);
	_partial_responses.erase(_client_fd);
}

bool RequestsManager::HandleRead() {
    try {
        char buffer[10000];
		int nbytes;

		nbytes = read (_client_fd, buffer, 10000);
		if (nbytes < 0)
			throw std::runtime_error("read error");
		else if (nbytes == 0) {
			/* End-of-file. */
			std::cerr << "Requests HandleRead: End-of-file\n";
			CloseClient();
			return 0;
		}
		else {
			std::cerr << "Requests HandleRead: Server: got message:" << buffer << "";
		}
    } catch (const std::exception& e) {
        std::cerr << "Error in Requests HandleRead: " << e.what() << std::endl;
        CloseClient();
		return 0;
    }
	return 1;
}

bool RequestsManager::HandleWrite() {
	std::string &response = _partial_responses[_client_fd];
	ssize_t bytes_written = write(_client_fd, response.c_str(), response.length());

	if (bytes_written <= 0) {
		if (errno != EAGAIN)
			CloseClient();
		return 0;
	}

	response.erase(0, bytes_written);

	if (response.empty()) {
		CloseClient();
		return 0;
	}
	return 1;
}

bool RequestsManager::HandleClient() {
	if (_client_fd == -1) {
		std::cerr << "Requests Error: No client_fd set\n";
		return 0;
	}
	if (POLLIN) {
		std::cerr << "Requests handle_client_read\n";
		return HandleRead();
	}
	if (POLLOUT) {
		std::cerr << "Requests handle_client_write\n";
		return HandleWrite();
	}
	if (POLLERR | POLLHUP | POLLNVAL) {
		std::cerr << "Requests else\n";
		CloseClient();
		return 0;
	}
	return 1;
}

void RequestsManager::CloseClient() {
	close (_client_fd);
	_partial_requests.erase(_client_fd);
	_partial_responses.erase(_client_fd);
}
