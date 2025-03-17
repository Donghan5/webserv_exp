#ifndef HTTPCONFIG_HPP
# define HTTPCONFIG_HPP

# include "AConfigBase.hpp"

/*
	Needed for sure:					Subject line
		error_pages						Your server must have default error pages if none are provided
		host and port					Choose the port and host of each ’server’.
		server_name						Set up the server_names or not
		max_body_size					Set the maximum allowed size for client request bodies
		allow_method					Define a list of accepted HTTP methods for the route.
		???	rewrite?					Define an HTTP redirect
		root							Define a directory or file where the requested file should be located (e.g.,
										if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet is
										/tmp/www/pouic/toto/pouet).
		autoindex						Enable or disable directory listing
		???								Set a default file to serve when the request is for a directory.
		?CGI							Execute CGI based on certain file extension (for example .php).
		???								Allow the route to accept uploaded files and configure where they should be
										saved.
*/

struct ServerConfig;

struct HttpConfig : AConfigBase
{
	STR						_global_user;
	STR						_global_worker_process;
	STR						_global_error_log;
	STR						_global_pid;

	int						_event_worker_connections;
	STR						_event_use;

	STR						_log_format;
	STR						_access_log;
	STR						_sendfile;
	STR						_keepalive_timeout;
	
	VECTOR<ServerConfig*>	_servers;
	void					_self_destruct();

	HttpConfig() :
        _global_user(""),
        _global_worker_process("1"),
        _global_error_log("logs/error.log"),
        _global_pid("logs/nginx.pid"),
        _event_worker_connections(1024),
        _event_use(""),
        _log_format("main"),
        _access_log("logs/access.log"),
        _sendfile("off"),
        _keepalive_timeout("65"),
		_servers()
    {
		_root = "./www";
	}
};

#endif