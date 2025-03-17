#ifndef RESPONSE_HPP
# define RESPONSE_HPP
# include "Request.hpp"
# include <iostream>

#include <cerrno>
#include <cstring> // For strerror

enum FileType {
    NotFound,
    NormalFile,
    Directory
};

class Response {
	private:
		Request		*_request;
		STR			_body;
		HttpConfig	*_config;
		std::map<STR, STR>	_all_mime_types; // _all_mime_types["extention"] = "name", _all_mime_types[".html"] = "text/html"
															// the order is not in reverse as several extentions can have same name
		
		std::map<int, STR>			_all_status_codes;
		STR	createResponse(int status, const STR& type, const STR& body);
		STR	handleGET(STR best_path, bool isDIR);
		STR	handlePOST(STR path);
		STR	handleDELETE(STR path);
		STR	getMime(STR path);
		STR	handleDIR(STR path);
		void		selectIndexIndexes(VECTOR<STR> indexes, STR &best_match, float &match_quality);
		STR	selectIndexAll(LocationConfig* location);
		FileType 	checkFile(const STR& path);
		LocationConfig *buildDirPath(ServerConfig *matchServer, STR &full_path, bool &isDIR);
		int			buildIndexPath(LocationConfig *matchLocation, STR &best_file_path);
		STR	matchMethod(STR path, bool isDIR);

	public:
		Response();
		Response(Request *request, HttpConfig *config);
		Response(const Response &obj);
		~Response();

		void	setRequest(Request *request);
		void	setConfig(HttpConfig *config);
		STR	getResponse();
};

#endif