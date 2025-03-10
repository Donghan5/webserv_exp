#include "LocationConfig.hpp"

LocationConfig::LocationConfig(/* args */) {
	_path = "";
	_add_header = "";
	_proxy_pass = "";
	_rewrite = "";
	_return = "";
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
	_rewrite = obj._rewrite;
	_return = obj._return;
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

LocationConfig::~LocationConfig() {

}