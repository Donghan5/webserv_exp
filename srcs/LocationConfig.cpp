#include "LocationConfig.hpp"

LocationConfig::LocationConfig(/* args */) {
	_path = "";
	_add_header = "";
	_proxy_pass = "";
	// _rewrite = "";
	// _return = "";
	_expires = "";
	_allow = "";
	_deny = "";
	_alias = "";
	_try_files = "";
	_root = "";
	_client_max_body_size = 1048576;
	_autoindex = false;
}

LocationConfig::LocationConfig(const LocationConfig &obj) {
	_path = obj._path;
	_add_header = obj._add_header;
	_proxy_pass = obj._proxy_pass;
	// _rewrite = obj._rewrite;
	// _return = obj._return;
	_expires = obj._expires;
	_allow = obj._allow;
	_deny = obj._deny;
	_alias = obj._alias;
	_try_files = obj._try_files;
	_root = obj._root;
	_autoindex = obj._autoindex;
	_client_max_body_size = obj._client_max_body_size;
	if (!obj._index.empty())
		_index = obj._index; //http, server
	else
		_index.clear();
	if (!obj._error_pages.empty())
		_error_pages = obj._error_pages; //http, server, location
	else
		_error_pages.clear();
	if (!obj._allowed_methods.empty())
		_allowed_methods = obj._allowed_methods;
	else
		_allowed_methods.clear();
	if (!obj._locations.empty())
		_locations = obj._locations;
	else
		_locations.clear();
}

LocationConfig &LocationConfig::operator=(const LocationConfig &obj) {
	return (*this);
}

LocationConfig::~LocationConfig() {}

void LocationConfig::setData(std::string key, std::string value) {
	if (!value.empty() && value[value.size() - 1] == ';') {  // extract semi-colon
		value.erase(value.size() - 1, 1);
	}
	this->_location_data[key] = value;
}

std::string LocationConfig::getData(std::string key) const {
	std::map<std::string, std::string>::const_iterator it = this->_location_data.find(key);
	if (it != _location_data.end()) {
		std::string value = it->second;
		return value;
	}
	return "";
}


/* 특별한 케이스를 제외하고 일반적인 경우에는 setter함수를 빼야하나? */
void LocationConfig::setPath(std::string path) {
	this->_path = path;
}

std::string LocationConfig::getPath(void) const {
	return _path;
}

void LocationConfig::setAddHeader(std::string add_header) {
	this->_add_header = add_header;
}

std::string LocationConfig::getAddHeader(void) const {
	return _add_header;
}

void LocationConfig::setProxyPass(std::string proxy_pass) {
	this->_proxy_pass = proxy_pass;
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

