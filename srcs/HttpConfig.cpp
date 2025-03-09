#include "HttpConfig.hpp"

HttpConfig::HttpConfig(/* args */) {
	_global_user = "";
	_global_worker_process = "";
	_global_error_log = "";
	_global_pid = "";
	_event_worker_connections = 1024;
	_event_use = "poll";
	_add_header = ""; //http, server, location
	_include = "";
	_log_format = "";
	_access_log = "";
	_sendfile = "";
	_keepalive_timeout = "";
	_gzip = "";
	_client_max_body_size = "";
	_root = "./www"; //http, server
}

HttpConfig::HttpConfig(const HttpConfig &obj) {
	_global_user = obj._global_user;
	_global_worker_process = obj._global_worker_process;
	_global_error_log = obj._global_error_log;
	_global_pid = obj._global_pid;
	_event_worker_connections = obj._event_worker_connections;
	_event_use = obj._event_use;
	_add_header = obj._add_header; //http, server, location
	_include = obj._include;
	_log_format = obj._log_format;
	_access_log = obj._access_log;
	_sendfile = obj._sendfile;
	_keepalive_timeout = obj._keepalive_timeout;
	_gzip = obj._gzip;
	_client_max_body_size = obj._client_max_body_size;
	_root = obj._root; //http, server
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
	if (!obj._servers.empty())
		_servers = obj._servers;
	else
		_servers.clear();
}

HttpConfig &HttpConfig::operator=(const HttpConfig &obj) {
	return (*this);
}

HttpConfig::~HttpConfig() {
	
}
