#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

void ServerConfig::_self_destruct() {
	for (std::map<STR, LocationConfig*>::iterator it = _locations.begin(); it != _locations.end(); ++it) {
		if (it->second)
			it->second->_self_destruct();
		it->second = NULL;
	}
	delete (this);
}

/* port value 0 to 65535 */
int ServerConfig::verifyPort(std::string port_str) {
	std::stringstream ss(port_str);
	int port;

	if (!(ss >> port) || port < 0 || port > 65335) {
		return -1;
	}
	return port;
}


/* max value 1048576 */
long long ServerConfig::verifyClientMaxBodySize(std::string client_max_body_size_str) {
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

	if (unit.empty() || unit == "b" || unit == "B") {
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
