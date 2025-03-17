#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP
# include "AConfigBase.hpp"

struct LocationConfig;

struct ServerConfig : AConfigBase {
	int								_listen_port;
	STR								_listen_server;
	STR								_location;
	STR 							_try_files; 			//server, location
	VECTOR<STR>						_server_name;

	MAP<STR, LocationConfig*>		_locations;
	void							_self_destruct();

	ServerConfig() :
        _listen_port(-1), //80 only if it's the only block
        _listen_server(""),
        _location(""),
        _try_files(""),
        _server_name()
    {
		_server_name.push_back("localhost");
		_server_name.push_back("127.0.0.1");
	}
};

#endif