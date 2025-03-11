#ifndef HTTPCONFIG_HPP
# define HTTPCONFIG_HPP
# include "ServerConfig.hpp"

class HttpConfig {
	private:
	
	public:
		HttpConfig();
		HttpConfig(const HttpConfig &obj);
		HttpConfig &operator=(const HttpConfig &obj);
		~HttpConfig();

		std::string					_global_user;
		std::string					_global_worker_process;
		std::string					_global_error_log;
		std::string					_global_pid;

		int							_event_worker_connections;
		std::string					_event_use;

		std::string					_add_header; //http, server, location
		std::string					_include;
		std::string					_log_format;
		std::string					_access_log;
		std::string					_sendfile;
		std::string					_keepalive_timeout;
		std::string					_gzip;
		long long					_client_max_body_size;  //http, server, location in bytes
		std::vector<std::string>	_server_name; //http, server
		std::string					_root; //http, server
		std::vector<std::string>	_index; //http, server
		std::map<int, std::string>	_error_pages; //http, server, location

		std::vector<ServerConfig*>	_servers;
};

#endif