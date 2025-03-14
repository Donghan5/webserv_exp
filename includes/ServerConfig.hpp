#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP
# include "LocationConfig.hpp"
# include "Utils.hpp"
# include <vector>

class HttpConfig;

class ServerConfig {
	private:

	public:
		// constructor and destructor
		ServerConfig();
		ServerConfig(const ServerConfig &obj);
		ServerConfig &operator=(const ServerConfig &obj);
		~ServerConfig();

		// data structure
		std::map<std::string, std::string> _server_data;

		// default params
		std::string								_add_header; //http, server, location
		int										_listen_port;
		std::string								_listen_server;
		std::string								_location; // Do I need it?
		std::string 							_try_files; //server, location
		std::vector<std::string>				_server_name; //http, server
		std::string								_root; //http, server, location
		std::vector<std::string>				_index; //http, server, location
		std::map<int, std::string>				_error_pages; //http, server, location
		long long								_client_max_body_size; //http, server, location in bytes

		std::map<std::string, LocationConfig*>	_locations;
		HttpConfig								*_back_ref; // Do I need it?

		// setter
		void setAddHeader(std::string add_header);
		void setListenPort(int listen_port);
		void setListenServer(std::string listen_server);
		void setLocation(std::string location);
		void setTryFiles(std::string try_files);
		void setServerName(std::vector<std::string> server_name);
		void setRoot(std::string root);
		void setLocations(std::map<std::string, LocationConfig*> locations);
		void setData(std::string key, std::string value);

		// getter
		std::string getAddHeader(void) const;
		int getListenPort(void) const;
		std::string getListenServer(void) const;
		std::string getLocation(void) const;
		std::string getTryFiles(void) const;
		std::vector<std::string> getServerName(void) const;
		std::string getRoot(void) const;
		std::map<std::string, LocationConfig*> getLocations(void) const;
		std::string getData(std::string key) const;

};

#endif
