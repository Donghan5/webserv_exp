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

ServerConfig::~ServerConfig() {
}
