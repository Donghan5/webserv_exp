#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP
# include <string>
# include <vector>
# include <map>

class ServerConfig;

class LocationConfig {
	private:
	
	public:
		LocationConfig();
		LocationConfig(const LocationConfig &obj);
		LocationConfig &operator=(const LocationConfig &obj);
		~LocationConfig();

		std::string	_path;
		std::string	_add_header; //http, server, location
		std::string	_proxy_pass;
		std::string	_rewrite;
		std::string	_return;
		std::string	_expires;
		std::string	_allow;
		std::string	_deny;
		std::string	_alias;
		std::string _try_files; //server, location
		std::string	_root; //http, server, location
		long long	_client_max_body_size; //http, server, location in bytes
		bool		_autoindex;
		
		std::vector<std::string>	_index; //http, server, location
		std::map<int, std::string>	_error_pages; //http, server, location
		std::vector<std::string>	_allowed_methods;

		std::map<std::string, LocationConfig>	_locations;
		ServerConfig	*back_ref;
};

#endif