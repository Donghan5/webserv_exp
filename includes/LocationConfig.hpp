#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP
# include "AConfigBase.hpp"

struct LocationConfig : AConfigBase {
	STR								_path;
	STR								_return;
	STR								_allow;
	STR								_deny;
	STR								_alias;
	STR 							_try_files; 			//server, location
	bool							_autoindex;
	VECTOR<STR>						_allowed_methods;
	MAP<STR, LocationConfig*>		_locations;

	void							_self_destruct();
	long long 						verifyClientMaxBodySize(std::string client_max_body_size_str);
	bool							verifyAutoIndex(std::string autoindex_str);

	LocationConfig() :
        _path(""),
        _return(""),
        _allow(""),
        _deny(""),
        _alias(""),
        _try_files(""),
        _autoindex(false),
        _allowed_methods()
    {}
};

#endif
