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
