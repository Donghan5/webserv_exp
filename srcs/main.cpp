// #include "PollServer.hpp"
// #include "SelectServer.hpp"
// #include "ServerConfig.hpp"
// #include "LocationConfig.hpp"

#include "HttpConfig.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "PollServer.hpp"
#include "Parser.hpp"

int	init_start_webserv(HttpConfig *config) {
	PollServer		poll_server;

		try {
			poll_server.setConfig(config);
		} catch (const std::exception& e) {
			std::cerr << "Poll Initialization Error: " << e.what() << std::endl;
			return 0;
		}

		try {
			poll_server.start();
		} catch (const std::exception& e) {
			std::cerr << "Poll Running Error: " << e.what() << std::endl;
			return 0;
		}
	return 0;
}

typedef std::map<int, STR> MAP_INT_STR;
typedef std::map<STR, LocationConfig*> MAP_STR_LOC;

// Printing functions
void printLocationConfig(const LocationConfig* loc, int indent = 2) {
    if (!loc) return;
    std::string pad(indent, ' ');
    std::cout << pad << "LocationConfig:\n";
    std::cout << pad << "  _proxy_pass_host: " << loc->_proxy_pass_host << "\n";
    std::cout << pad << "  _proxy_pass_port: " << loc->_proxy_pass_port << "\n";
    std::cout << pad << "  _path: " << loc->_path << "\n";
    std::cout << pad << "  _add_header: " << loc->_add_header << "\n";
    std::cout << pad << "  _return_code: " << loc->_return_code << "\n";
    std::cout << pad << "  _return_url: " << loc->_return_url << "\n";
    std::cout << pad << "  _root: " << loc->_root << "\n";
    std::cout << pad << "  _client_max_body_size: " << loc->_client_max_body_size << "\n";
    std::cout << pad << "  _autoindex: " << (loc->_autoindex ? "true" : "false") << "\n";

    std::cout << pad << "  _index: [";
    for (VECTOR<STR>::const_iterator it = loc->_index.begin(); it != loc->_index.end(); ++it) {
        std::cout << (it != loc->_index.begin() ? ", " : "") << *it;
    }
    std::cout << "]\n";

    std::cout << pad << "  _error_pages:\n";
    for (MAP_INT_STR::const_iterator it = loc->_error_pages.begin(); it != loc->_error_pages.end(); ++it) {
        std::cout << pad << "    " << it->first << ": " << it->second << "\n";
    }

    std::cout << pad << "  _allowed_methods: [";
    for (MAP<STR, bool>::const_iterator it = loc->_allowed_methods.begin(); it != loc->_allowed_methods.end(); ++it) {
        if (it->second) {
            std::cout << (it != loc->_allowed_methods.begin() ? ", " : "") << it->first;
        }
    }
    std::cout << "]\n";

    std::cout << pad << "  _locations:\n";
    for (MAP_STR_LOC::const_iterator it = loc->_locations.begin(); it != loc->_locations.end(); ++it) {
        std::cout << pad << "    " << it->first << ":\n";
        printLocationConfig(it->second, indent + 4);  // Recursive call
    }
}

void printServerConfig(const ServerConfig* server, int indent = 2) {
    if (!server) return;
    std::string pad(indent, ' ');
    std::cout << pad << "ServerConfig:\n";
    std::cout << pad << "  _add_header: " << server->_add_header << "\n";
    std::cout << pad << "  _listen_port: " << server->_listen_port << "\n";
    std::cout << pad << "  _listen_server: " << server->_listen_server << "\n";
    std::cout << pad << "  _root: " << server->_root << "\n";
    std::cout << pad << "  _client_max_body_size: " << server->_client_max_body_size << "\n";

    std::cout << pad << "  _server_name: [";
    for (VECTOR<STR>::const_iterator it = server->_server_name.begin(); it != server->_server_name.end(); ++it) {
        std::cout << (it != server->_server_name.begin() ? ", " : "") << *it;
    }
    std::cout << "]\n";

    std::cout << pad << "  _index: [";
    for (VECTOR<STR>::const_iterator it = server->_index.begin(); it != server->_index.end(); ++it) {
        std::cout << (it != server->_index.begin() ? ", " : "") << *it;
    }
    std::cout << "]\n";

    std::cout << pad << "  _error_pages:\n";
    for (MAP_INT_STR::const_iterator it = server->_error_pages.begin(); it != server->_error_pages.end(); ++it) {
        std::cout << pad << "    " << it->first << ": " << it->second << "\n";
    }

    std::cout << pad << "  _locations:\n";
    for (MAP_STR_LOC::const_iterator it = server->_locations.begin(); it != server->_locations.end(); ++it) {
        std::cout << pad << "    " << it->first << ":\n";
        printLocationConfig(it->second, indent + 4);  // Recursive call
    }
}

void printHttpConfig(const HttpConfig& http, int indent = 0) {
    std::string pad(indent, ' ');
    std::cout << pad << "HttpConfig:\n";
    std::cout << pad << "  _global_user: " << http._global_user << "\n";
    std::cout << pad << "  _global_worker_process: " << http._global_worker_process << "\n";
    std::cout << pad << "  _global_error_log: " << http._global_error_log << "\n";
    std::cout << pad << "  _global_pid: " << http._global_pid << "\n";
    std::cout << pad << "  _event_worker_connections: " << http._event_worker_connections << "\n";
    std::cout << pad << "  _event_use: " << http._event_use << "\n";
    std::cout << pad << "  _keepalive_timeout: " << http._keepalive_timeout << "\n";
    std::cout << pad << "  _add_header: " << http._add_header << "\n";
    std::cout << pad << "  _client_max_body_size: " << http._client_max_body_size << "\n";
    std::cout << pad << "  _root: " << http._root << "\n";

    std::cout << pad << "  _index: [";
    for (VECTOR<STR>::const_iterator it = http._index.begin(); it != http._index.end(); ++it) {
        std::cout << (it != http._index.begin() ? ", " : "") << *it;
    }
    std::cout << "]\n";

    std::cout << pad << "  _error_pages:\n";
    for (MAP_INT_STR::const_iterator it = http._error_pages.begin(); it != http._error_pages.end(); ++it) {
        std::cout << pad << "    " << it->first << ": " << it->second << "\n";
    }

    std::cout << pad << "  _servers:\n";
    for (VECTOR<ServerConfig*>::const_iterator it = http._servers.begin(); it != http._servers.end(); ++it) {
        std::cout << pad << "    Server #" << (it - http._servers.begin()) << ":\n";
        printServerConfig(*it, indent + 4);  // Recursive call
    }
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Error: no config file\n";
		exit (1);
	}

	Parser parser(argv[1]);

	HttpConfig *newConf = NULL;

    try {
        newConf = parser.Parse();
    } catch (const std::exception& e) {
        std::cerr << "ERROR Parsing failure: " << e.what() << std::endl;
        return 1;
    }

	if (!newConf) {
        std::cerr << "ERROR Parsing failed\n";
        return 1;
    }

	printHttpConfig(*newConf);

    try {
        init_start_webserv(newConf);
    } catch (const std::exception& e) {
        std::cerr << "ERROR Running failure: " << e.what() << std::endl;
        newConf->_self_destruct();
        return 1;
    }

    newConf->_self_destruct();
    return 0;
}
