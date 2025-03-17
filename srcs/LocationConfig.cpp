#include "LocationConfig.hpp"

void LocationConfig::_self_destruct() {
	for (std::map<STR, LocationConfig*>::iterator it = _locations.begin(); it != _locations.end(); ++it) {
		if (it->second)
			it->second->_self_destruct();
		it->second = NULL;
	}
	delete (this);
}

/* 특별한 케이스를 제외하고 일반적인 경우에는 setter함수를 빼야하나? */
void LocationConfig::setPath(std::string path) {
	this->_path = path;
}

std::string LocationConfig::getPath(void) const {
	return LocationConfig::_path;
}

void LocationConfig::setAddHeader(std::string add_header) {
	this->_add_header = add_header;
}

std::string LocationConfig::getAddHeader(void) const {
	return _add_header;
}

void LocationConfig::setProxyPass(std::string proxy_pass) {
	_proxy_pass = proxy_pass;
}

std::string LocationConfig::getProxyPass(void) const {
	return _proxy_pass;
}

void LocationConfig::setExpires(std::string expires) {
	this->_expires = expires;
}

std::string LocationConfig::getExpires(void) const {
	return _expires;
}

void LocationConfig::setAllow(std::string allow) {
	this->_allow = allow;
}

std::string LocationConfig::getAllow(void) const {
	return _allow;
}

void LocationConfig::setDeny(std::string deny) {
	this->_deny = deny;
}

std::string LocationConfig::getDeny(void) const {
	return _deny;
}

void LocationConfig::setAlias(std::string alias) {
	this->_alias = alias;
}

std::string LocationConfig::getAlias(void) const {
	return _alias;
}

void LocationConfig::setTryFiles(std::string try_files) {
	this->_try_files = try_files;
}

std::string LocationConfig::getTryFiles(void) const {
	return _try_files;
}

void LocationConfig::setRoot(std::string root) {
	root = getData("root");
	this->_root = root;
}

std::string LocationConfig::getRoot(void) const {
	return _root;
}

void LocationConfig::setClientMaxBodySize(std::string client_max_body_size_str) {
	long long client_max_body_size = std::atoll(client_max_body_size_str.c_str());

	if (client_max_body_size <= 0) {
		throw std::logic_error("Invalid body_size value");
		return ;
	}
	this->_client_max_body_size = client_max_body_size;
}

long long LocationConfig::getClientMaxBodySize(void) const {
	return _client_max_body_size;
}

void LocationConfig::setAutoIndex(std::string autoindex_str) {
	autoindex_str = getData("autoindex");
	bool autoindex;
	if (autoindex_str == "on") {
		autoindex = true;
	}
	else if (autoindex_str == "off") {
		autoindex = false;
	}
	this->_autoindex = autoindex;
}

bool LocationConfig::getAutoIndex(void) const {
	return _autoindex;
}

