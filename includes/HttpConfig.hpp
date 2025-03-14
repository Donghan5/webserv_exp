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

		// event block settings
		int							_event_worker_connections;
		std::string					_event_use; // epoll, kqueue

		// http settings
		std::string					_add_header; //http, server, location
		std::string					_include;
		std::string					_log_format;
		std::string					_access_log;
		std::string					_sendfile; // do I need this ?
		std::string					_keepalive_timeout;
		std::string					_gzip; // do I need this ?
		long long					_client_max_body_size;  //http, server, location in bytes
		std::vector<std::string>	_server_name; //http, server
		std::string					_root; //http, server
		std::vector<std::string>	_index; //http, server
		std::map<int, std::string>	_error_pages; //http, server, location

		// server list
		std::vector<ServerConfig*>	_servers;

		// data structures
		std::map<std::string, std::string> _http_data;

		// setter
		void setGlobalUser(std::string global_user);
		void setGlobalWorkerProcess(std::string global_worker_process);
		void setGlobalErrorLog(std::string global_error_log);
		void setGlobalPid(std::string global_pid);
		void setEventWorkerConnections(int event_worker_connections);
		void setEventUse(std::string event_use);
		void setAddHeader(std::string add_header);
		void setInclude(std::string include);
		void setLogFormat(std::string log_format);
		void setAccessLog(std::string access_log);
		void setSendFile(std::string sendfile);
		void setKeepaliveTimeout(std::string keepalive_timeout);
		void setGzip(std::string gzip);
		void setClientMaxBodySize(long long client_max_body_size);
		void setServerName(std::vector<std::string> server_name);
		void setRoot(std::string root);
		void setIndex(std::vector<std::string> index);
		void setErrorPages(std::map<int, std::string> error_pages);
		void setServers(std::vector<ServerConfig*> servers);
		void setData(std::string key, std::string value);

		// getter
		std::string getGlobalUser(void) const;
		std::string getGlobalWorkerProcess(void) const;
		std::string getGlobalErrorLog(void) const;
		std::string getGlobalPid(void) const;
		int getEventWorkerConnections(void) const;
		std::string getEventUse(void) const;
		std::string getAddHeader(void) const;
		std::string getInclude(void) const;
		std::string getLogFormat(void) const;
		std::string getAccessLog(void) const;
		std::string getSendFile(void) const;
		std::string getKeepaliveTimeout(void) const;
		std::string getGzip(void) const;
		long long getClientMaxBodySize(void) const;
		std::vector<std::string> getServerName(void) const;
		std::string getRoot(void) const;
		std::vector<std::string> getIndex(void) const;
		std::map<int, std::string> getErrorPages(void) const;
		std::vector<ServerConfig*> getServers(void) const;
		std::string getData(std::string key) const;
};

#endif
