#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP
# include "AConfigBase.hpp"

/* server and location return */
struct LocationConfig : AConfigBase {
	STR								_proxy_pass_host;
	int								_proxy_pass_port;
	STR								_path;
	int								_return_code; // return code
	STR								_return_url;
	STR								_allow;
	STR								_deny;
	STR								_alias;
	STR 							_try_files; 			//server, location
	bool							_autoindex;
	MAP<STR, bool>					_allowed_methods;
	MAP<STR, LocationConfig*>		_locations;

	void							_self_destruct();

	LocationConfig() :
		_proxy_pass_host(""),
		_proxy_pass_port(8080),
        _path(""),
		_return_code(-1),
        _return_url(""),
        _allow(""),
        _deny(""),
        _alias(""),
        _try_files(""),
        _autoindex(false)
    {
		_allowed_methods["GET"] = false;
		_allowed_methods["POST"] = false;
		_allowed_methods["DELETE"] = false;
	}
};

#endif
