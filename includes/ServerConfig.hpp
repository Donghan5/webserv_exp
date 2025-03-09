#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP
# include "LocationConfig.hpp"

class ServerConfig {
	private:

	public:
		ServerConfig();
		ServerConfig(const ServerConfig &obj);
		ServerConfig &operator=(const ServerConfig &obj);
		~ServerConfig();

		std::string					_add_header; //http, server, location
		int							_listen_port;
		std::string					_listen_server;
		std::string					_location;
		std::string 				_try_files; //server, location
		std::vector<std::string>	_server_name; //http, server
		std::string					_root; //http, server, location
		std::vector<std::string>	_index; //http, server, location
		std::map<int, std::string>	_error_pages; //http, server, location

		std::map<std::string, LocationConfig>	_locations;
};

#endif