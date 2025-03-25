#include "Request.hpp"
#include <sstream>

void remove_trailing_r(STR &str) {
	if (!str.empty() && str[str.length() - 1] == '\r') {
		str.replace(str.length() - 1, 1, "\0");
	}
}

void	process_path(STR &full_path, STR &file_name) {
	std::cerr << "Full path before: " << full_path << "\n";

	if (full_path.find('.') != STR::npos) {
		file_name = full_path.substr(full_path.find_last_of('/') + 1, full_path.size());
		full_path = full_path.substr(0, full_path.find_last_of('/') + 1);
	} else {
		file_name = "";
	}

	std::cerr << "Full path after: " << full_path << "\n";
	std::cerr << "File name after: " << file_name << "\n";
}

bool Request::parseHeader() {
	std::istringstream	request_stream(_full_request);
    STR					temp_line;
	std::istringstream	temp_line_stream;
	STR 				temp_token;

	if (_full_request == "")
		return false;

	//parse first line		GET / HTTP/1.1
	std::getline(request_stream, temp_line);
	remove_trailing_r(temp_line);
	temp_line_stream.str(temp_line);
	getline(temp_line_stream, temp_token, ' ');
	_method = temp_token;
	getline(temp_line_stream, temp_token, ' ');
	_file_path = temp_token;
	parseQueryString(); // to parse query string, if it exists
	getline(temp_line_stream, temp_token, ' ');
	_http_version = temp_token;

	//parse the rest of request searching for data nedded
	while (std::getline(request_stream, temp_line)) {
		if (temp_line.find("\r\n\r\n") != STR::npos) {
			return true;
		}
		remove_trailing_r(temp_line);
		temp_line_stream.clear();
		temp_token.clear();
		temp_line_stream.str(temp_line);
		getline(temp_line_stream, temp_token, ' ');

		//searching for 	Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
		if (temp_token == "Accept:") {
			STR			filetypes_line;
			std::istringstream	filetypes_stream;
			STR 		filetype_token;

			//getting the second part of line line with filetypes
			getline(temp_line_stream, filetypes_line, ' ');
			filetypes_stream.str(filetypes_line);

			//getting types from type line one by one
			while (getline(filetypes_stream, filetype_token, ',')) {
				std::istringstream	name_quality_stream;
				STR 		name_quality_token;
				STR			type_name;
				float				type_quality = 1.0;

				//separating by name and value (quality/preference)
				name_quality_stream.str(filetype_token);
				getline(name_quality_stream, name_quality_token, ';');
				type_name = name_quality_token.c_str();
				type_name += '\0';
				if (getline(name_quality_stream, name_quality_token, ';'))
					type_quality = atof(name_quality_token.c_str() + 2);
				_accepted_types[type_name.c_str()] = type_quality;
			}
		} else if (temp_token == "Cookie:") {
			//to do cookies
			_cookies = temp_line.substr(temp_line.find_first_of(':') + 2);
		} else if (temp_token == "Host:") {
			int	host_start;
			int	host_end;
			int	delim_position;
			int	port_end;

			//extracting host and port from 		Host: localhost:8080
			host_start = temp_line.find_first_of(':') + 2;
			delim_position = temp_line.find(':', temp_line.find_first_of(':') + 1);

			if (delim_position == (int)STR::npos) { //Not tested
			//host without port
				host_end = temp_line.find_last_not_of(' '); //		Future check required: newline included or not to remove +1
				_host = temp_line.substr(host_start, host_end - host_start);
			} else {
			//host with port
				host_end = delim_position;
				port_end = temp_line.find_last_not_of(' ');
				_host = temp_line.substr(host_start, host_end - host_start);
				_port = atoi(temp_line.substr(delim_position + 1, delim_position + 1 - port_end).c_str());
			}
		} else if (temp_token == "Content-Type:") {	//Not tested
			int	delim_position;
			int	end_position;

			delim_position = temp_line.find_first_of(':');
			end_position = temp_line.find_last_not_of(' ');
			_content_type = temp_line.substr(delim_position + 2, end_position - delim_position - 2);
			std::cerr << "cont type = " << _content_type << "\n";
		} else if (temp_token == "Content-Length:") {
			int	delim_position;
			int	end_position;

			delim_position = temp_line.find_first_of(':');
			end_position = temp_line.length();
			_body_size = atoi((temp_line.substr(delim_position + 1, delim_position + 1 - end_position)).c_str());
		}
		temp_line.clear();
		temp_token.clear();
    }

	// process_path(_file_path, _file_name);
	return true;
}

bool Request::parseBody() {
	int	body_beginning = -1;
	if (_full_request == "")
		return false;

	body_beginning = _full_request.find("\r\n\r\n");
	if (body_beginning == CHAR_NOT_FOUND)
		return false;
	_body = _full_request.substr(body_beginning + 4, _full_request.length() - (body_beginning + 4));

	//if _full_request.length() - (body_beginning + 4) != _body_size 		potential error

	// std::cerr << "DEBUG Requet::parseBody _body = |" << _body << "|\n";
	return true;
}


bool Request::parseRequest() {
	if (!parseHeader())
		return false;
	if (_body_size && !parseBody())
		return false;
	return true;
}

void Request::parseQueryString(void) {
	size_t query_pos = _file_path.find('?');

	if (query_pos != STR::npos) {
		_query_string = _file_path.substr(query_pos + 1);
		_file_path = _file_path.substr(0, query_pos);
	}
	else {
		_query_string = "";
	}
}

Request::Request() {
	_cookies = "";
	_full_request = "";
	_file_path = "";
	_method = "";
	_http_version = "";
	_host = "localhost";
	_port = 80;
	_content_type = "";
	_body = "";
	_body_size = 0;
}

Request::Request(STR request) {
	_full_request = request;
	_cookies = "";
	_file_path = "";
	_method = "";
	_http_version = "";
	_host = "localhost";
	_port = 80;
	_content_type = "";
	_body = "";
	_body_size = 0;
	parseRequest();
}

Request::Request(const Request &obj) {
	_cookies = obj._cookies;
	_full_request = obj._full_request;
	_file_path = obj._file_path;
	_method = obj._method;
	_http_version = obj._http_version;
	_host = obj._host;
	_port = obj._port;
	_content_type = obj._content_type;
	_accepted_types = obj._accepted_types;
	_body = obj._body;
	_body_size = obj._body_size;
}

Request::~Request() {

}

void Request::setRequest(STR request) {
	_full_request = request;
	_body = "";

	if (!parseRequest()) {
		std::cerr << "Request::setRequest: Request Parsing error!\n";
	}
}
