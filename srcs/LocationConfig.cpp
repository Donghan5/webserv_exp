#include "LocationConfig.hpp"

void LocationConfig::_self_destruct() {
	for (std::map<STR, LocationConfig*>::iterator it = _locations.begin(); it != _locations.end(); ++it) {
		if (it->second)
			it->second->_self_destruct();
		it->second = NULL;
	}
	delete (this);
}

/* max value 1048576 */
long long LocationConfig::verifyClientMaxBodySize(std::string client_max_body_size_str) {
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

	if (unit.empty() || unit == "b" || unit == "B") { // byte, don't need to convert
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

	if (value > LLONG_MAX) { // check the overflow
		return -1;
	}

	return value;
}

bool LocationConfig::verifyAutoIndex(std::string autoindex_str) {
	bool autoindex = false;
	if (autoindex_str == "on") {
		autoindex = true;
	}
	else if (autoindex_str == "off") {
		autoindex = false;
	}
	return autoindex;
}
