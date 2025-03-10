#ifndef RESPONSE_HPP
# define RESPONSE_HPP
# include "Request.hpp"
# include <iostream>

class Response {
	private:
		Request		*_request;
		HttpConfig	*_config;
		std::map<std::string, std::string>	_all_mime_types; // _all_mime_types["extention"] = "name", _all_mime_types[".html"] = "text/html"
															// the order is not in reverse as several extentions can have same name
		
		std::map<int, std::string>			_all_status_codes;
		std::string	createResponse(int status, const std::string& type, const std::string& body);
		std::string	handleGET(LocationConfig* location);
	public:
		Response();
		Response(Request *request, HttpConfig *config);
		Response(const Response &obj);
		~Response();

		void	setRequest(Request *request);
		void	setConfig(HttpConfig *config);
		std::string	getResponse();
};

#endif