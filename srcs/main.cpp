#include "PollServer.hpp"
#include "SelectServer.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

int	init_start_webserv(HttpConfig *config) {
	SelectServer	select_server;
	PollServer		poll_server;

	if (config->_event_use == "select") {
		try {
			select_server.setConfig(config);
		} catch (const std::exception& e) {
			std::cerr << "Select Initialization Error: " << e.what() << std::endl;
			return 0;
		}
	
		try {
			select_server.start();
		} catch (const std::exception& e) {
			std::cerr << "Select Running Error: " << e.what() << std::endl;
			return 0;
		}
	} else {
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
	}
	return 0;
}

int main() {
	HttpConfig			config;
	ServerConfig		server;
	LocationConfig		l1, l2;

	try {
		config._event_worker_connections = 1024;
		config._event_use = "poll";
		// config._event_use = "select";
		config._root = "./www3";
		
		server._listen_port = 8080;
		server._server_name.push_back("localhost");
		server._root = "./www3";
		server._index.push_back("index.html");
		
		
		l1._path = "/";
		l1._root = "./www3";
		l1._index.push_back("index.html");
		l1._allowed_methods.push_back("GET");
		
		l2._path = "/uploads";
		l2._allowed_methods.push_back("GET");
		l2._allowed_methods.push_back("POST");
		l2._allowed_methods.push_back("DELETE");
		l2._autoindex = true;

		server._locations[l1._path] = l1;
		server._locations[l2._path] = l2;
		config._servers.push_back(server);
	} catch (const std::exception& e) {
		std::cerr << "Parsing Error: " << e.what() << std::endl;
		return 1;
	}

	init_start_webserv(&config);

    return 0;
}
