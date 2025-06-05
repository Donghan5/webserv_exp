#include "ParserUtils.hpp"

int ParserUtils::verifyPort(std::string port_str) {
	std::stringstream ss(port_str);
	int port;

	if (!(ss >> port) || port < 0 || port > 65335) {
		return -1;
	}
	return port;
}

bool ParserUtils::verifyAutoIndex(STR autoindex_str) {
	bool autoindex = false;
	if (autoindex_str == "on") {
		autoindex = true;
	}
	else if (autoindex_str == "off") {
		autoindex = false;
	}
	return autoindex;
}

/*
 * client_max_body_size we use MB NOT MiB
 * 1b = 1 byte
 * 1k = 1000 bytes
 * 1m = 1000 * 1000 bytes
 * 1g = 1000 * 1000 * 1000 bytes
 *
 * default is 1M
*/
long long ParserUtils::verifyClientMaxBodySize(std::string client_max_body_size_str) {
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
		value *= 1000;
	}
	else if (unit == "m" || unit == "M") {
		value *= 1000 * 1000;
	}
	else if (unit == "g" || unit == "G") {
		value *= 1000 * 1000 * 1000;
	}

	if (value > LLONG_MAX) {
		return -1;
	}

	return value;
}

bool ParserUtils::isDirectiveOk(STR line, int start, int end) {
	VECTOR<STR>	tokens;
	STR			trimmed_line;

	trimmed_line = line.substr(start, end - start);
	// std::cerr << "TRIMMED |" << trimmed_line << "|\n";
	tokens = Utils::split(trimmed_line, ' ', 1);
	if (tokens.size() < 2)
		return false;
	return true;
}

bool ParserUtils::isBlockOk(STR line, int start, int end) {
	VECTOR<STR>	tokens;
	STR			trimmed_line;
	STR			block_name;

	trimmed_line = line.substr(start, end - start + 1);

	block_name = trimmed_line.substr(0, trimmed_line.find('{'));

	if (block_name == "")
		return false;

	//checking text before block
	tokens = Utils::split(block_name, ' ', 1);
	if (tokens.size() != 1 && tokens.size() != 2)
		return false;

	if (tokens[0] != "events" && tokens[0] != "http" && tokens[0] != "server" && tokens[0] != "location")
		return false;

	if (tokens[0] == "location" && tokens.size() != 2)
		return false;

	if (tokens[0] != "location" && tokens.size() != 1)
		return false;

	return true;
}

bool	ParserUtils::isBlockEndOk(STR line, int start) {
	std::istringstream	string_stream;
	STR					trimmed_line;
	STR					no_ws;

	trimmed_line = line.substr(start);

	string_stream.str(trimmed_line);
	string_stream >> no_ws;
	if (no_ws[0] != '}')
		return false;
	return true;
}
