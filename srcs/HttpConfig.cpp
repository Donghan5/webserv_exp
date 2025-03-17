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

void HttpConfig::setGlobalUser(std::string global_user) {
	this->_global_user = global_user;
}

std::string HttpConfig::getGlobalUser(void) const {
	return _global_user;
}

void HttpConfig::setGlobalWorkerProcess(std::string global_worker_process) {
	this->_global_worker_process = global_worker_process;
}

std::string HttpConfig::getGlobalWorkerProcess(void) const {
	return _global_worker_process;
}

void HttpConfig::setGlobalErrorLog(std::string global_error_log) {
	this->_global_error_log = global_error_log;
}

std::string HttpConfig::getGlobalErrorLog(void) const {
	return _global_error_log;
}

void HttpConfig::setGlobalPid(std::string global_pid) {
	this->_global_pid = global_pid;
}

std::string HttpConfig::getGlobalPid(void) const {
	return _global_pid;
}

void HttpConfig::setEventWorkerConnections(int event_worker_connections) {
	this->_event_worker_connections = event_worker_connections;
}

int HttpConfig::getEventWorkerConnections(void) const {
	return _event_worker_connections;
}

void HttpConfig::setEventUse(std::string event_use) {
	this->_event_use = event_use;
}

std::string HttpConfig::getEventUse(void) const {
	return _event_use;
}

void HttpConfig::setAddHeader(std::string add_header) {
	this->_add_header = add_header;
}

std::string HttpConfig::getAddHeader(void) const {
	return _add_header;
}

void HttpConfig::setInclude(std::string include) {
	this->_include = include;
}

std::string HttpConfig::getInclude(void) const {
	return _include;
}

void HttpConfig::setLogFormat(std::string log_format) {
	this->_log_format = log_format;
}

std::string HttpConfig::getLogFormat(void) const {
	return _log_format;
}

void HttpConfig::setAccessLog(std::string access_log) {
	this->_access_log = access_log;
}

std::string HttpConfig::getAccessLog(void) const {
	return _access_log;
}

void HttpConfig::setSendFile(std::string sendfile) {
	this->_sendfile = sendfile;
}

std::string HttpConfig::getSendFile(void) const {
	return _sendfile;
}

void HttpConfig::setKeepaliveTimeout(std::string keepalive_timeout) {
	this->_keepalive_timeout = keepalive_timeout;
}

std::string HttpConfig::getKeepaliveTimeout(void) const {
	return _keepalive_timeout;
}

void HttpConfig::setGzip(std::string gzip) {
	this->_gzip = gzip;
}

std::string HttpConfig::getGzip(void) const {
	return _gzip;
}

void HttpConfig::setClientMaxBodySize(std::string client_max_body_size_str) {
	long long client_max_body_size = std::atoll(client_max_body_size_str.c_str());

	if (client_max_body_size <= 0) {
		throw std::logic_error("Invalid body_size value");
		return ;
	}
	this->_client_max_body_size = client_max_body_size;
}

long long HttpConfig::getClientMaxBodySize(void) const {
	return _client_max_body_size;
}

void HttpConfig::setServerName(std::vector<std::string> server_name) {
	this->_server_name = server_name;
}

std::vector<std::string> HttpConfig::getServerName(void) const {
	return _server_name;
}

void HttpConfig::setRoot(std::string root) {
	this->_root = root;
}

std::string HttpConfig::getRoot(void) const {
	return _root;
}

void HttpConfig::setIndex(std::vector<std::string> index) {
	this->_index = index;
}

std::vector<std::string> HttpConfig::getIndex(void) const {
	return _index;
}

void HttpConfig::setErrorPages(std::map<int, std::string> error_pages) {
	this->_error_pages = error_pages;
}

std::map<int, std::string> HttpConfig::getErrorPages(void) const {
	return _error_pages;
}

void HttpConfig::setServers(std::vector<ServerConfig*> servers) {
	this->_servers = servers;
}

std::vector<ServerConfig*> HttpConfig::getServers(void) const {
	return _servers;
}

void HttpConfig::setRoot(std::string root) {
	root = getData("root");
	this->_root = root;
}

std::string HttpConfig::getRoot(void) const {
	return _root;
}
