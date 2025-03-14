#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {
	_add_header = ""; //http, server, location
	_listen_port = 8080;
	_listen_server = "";
	_location = "";
	_try_files = "";
	_root = ""; //http, server
	_client_max_body_size = 1048576;
}

ServerConfig::ServerConfig(const ServerConfig &obj) {
	_add_header = obj._add_header; //http, server, location
	_listen_port = obj._listen_port;
	_listen_server = obj._listen_server;
	_location = obj._location;
	_try_files = obj._try_files;
	_root = obj._root; //http, server
	_client_max_body_size = obj._client_max_body_size;
	if (!obj._server_name.empty())
		_server_name = obj._server_name; //http, server
	else
		_server_name.clear();
	if (!obj._index.empty())
		_index = obj._index; //http, server
	else
		_index.clear();
	if (!obj._error_pages.empty())
		_error_pages = obj._error_pages; //http, server, location
	else
		_error_pages.clear();
	if (!obj._locations.empty())
		_locations = obj._locations; //potential copy
	else
		_locations.clear();
}

ServerConfig &ServerConfig::operator=(const ServerConfig &obj) {
	return (*this);
}

ServerConfig::~ServerConfig() {}

void ServerConfig::setData(std::string key, std::string value) {
	if (!value.empty() && value[value.size() - 1] == ';') {  // extract semi-colon
		value.erase(value.size() - 1, 1);
	}
	this->_server_data[key] = value;
}

std::string ServerConfig::getData(std::string key) const {
	std::map<std::string, std::string>::const_iterator it = this->_server_data.find(key);
	if (it != _server_data.end()) {
		std::string value = it->second;
		return value;
	}
	return "";
}

void ServerConfig::setAddHeader(std::string add_header) {
	add_header = getData("add_header");
	this->_add_header = add_header;
}

std::string ServerConfig::getAddHeader(void) const {
	return _add_header;
}

void ServerConfig::setListenPort(int listen_port) {
	listen_port = std::atoi(getData("port").c_str());
	this->_listen_port = listen_port;
}

int ServerConfig::getListenPort(void) const {
	return _listen_port;
}

void ServerConfig::setListenServer(std::string listen_server) {
	// ??
}

std::string ServerConfig::getListenServer(void) const {
	// ??
}

void ServerConfig::setLocation(std::string location) {
	// ??
}

std::string ServerConfig::getLocation(void) const {
	// ??
}

void ServerConfig::setServerName(std::vector<std::string> server_name) {
	std::string server_name_str = getData("server_name");
	server_name = Utils::split(server_name_str, ' ');
	this->_server_name = server_name;
}

std::vector<std::string> ServerConfig::getServerName(void) const {
	return _server_name;
}

void ServerConfig::setRoot(std::string root) {
	root = getData("root");
}
