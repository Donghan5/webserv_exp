#include "HttpConfig.hpp"
#include "ServerConfig.hpp"

void HttpConfig::_self_destruct() {
	for (size_t i = 0; i < _servers.size(); i++) {
		if (_servers[i])
			_servers[i]->_self_destruct();
		_servers[i] = NULL;
	}
	delete (this);
}

/* default limit 512 */
int HttpConfig::veriftEventWorkerConnections(std::string event_worker_connections_str) {
	std::stringstream ss(event_worker_connections_str);
	int value;

	for (size_t i = 0; i < event_worker_connections_str.length(); i++) {
		if (!std::isdigit(event_worker_connections_str[i])) {
			return -1;
		}
	}

	if (!(ss >> value) || value <= 0) {
		return -1;
	}

	return value;
}

/* max value 1048576 */
long long HttpConfig::verifyClientMaxBodySize(std::string client_max_body_size_str) {
	std::stringstream ss(client_max_body_size_str);
	long long value;
	std::string unit;

	if (!(ss >> value)) {
		return -1;
	}
	ss >> unit;

	if (value < 0) {
		return -1;
	}

	if (unit.empty() || unit == "b" || unit == "B") { // default byte
		value *= 1;
	}
	else if (unit == "k" || unit == "K") {
		value *= 1024;
	}
	else if (unit == "m" || unit == "M") {
		value *= 1024 * 1024;
	}
	else if (unit == "g" || unit == "G") {
		value *= 1024 * 1024 * 1024;
	}

	if (value > LLONG_MAX) {
		return -1;
	}

	return value;
}
