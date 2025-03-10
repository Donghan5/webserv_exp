#include "Request.hpp"
#include <sstream>

bool Request::parseRequest() {
	std::istringstream	request_stream(_full_request);
    std::string			temp_line;
	std::istringstream	temp_line_stream;
	std::string 		temp_token;

	if (_full_request == "")
		return false;

	//parse first line		GET / HTTP/1.1
	std::getline(request_stream, temp_line);
	temp_line_stream.str(temp_line);
	getline(temp_line_stream, temp_token, ' ');
	_method = temp_token;
	getline(temp_line_stream, temp_token, ' ');
	_file_path = temp_token;
	getline(temp_line_stream, temp_token, ' ');
	_http_version = temp_token;

	// parse first line (GET / HTTP/1.1) different way
	// if (std::getline(temp_line_stream, temp_line)) {
	// 	std::istringstream line_stream(temp_line);
	// 	line_stream >> _method >> _file_path >> _http_version;
	// }

	//parse the rest of request searching for data nedded
	while (std::getline(request_stream, temp_line)) {
		temp_line_stream.clear();
		temp_token.clear();
		temp_line_stream.str(temp_line);
		getline(temp_line_stream, temp_token, ' ');

		//searching for 	Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
		if (temp_token == "Accept:") {
			std::string			filetypes_line;
			std::istringstream	filetypes_stream;
			std::string 		filetype_token;

			//getting the second part of line line with filetypes
			getline(temp_line_stream, filetypes_line, ' ');
			filetypes_stream.str(filetypes_line);

			//getting types from type line one by one
			while (getline(filetypes_stream, filetype_token, ',')) {
				std::istringstream	name_quality_stream;
				std::string 		name_quality_token;
				std::string			type_name;
				float				type_quality = 1.0;

				//separating by name and value (quality/preference)
				name_quality_stream.str(filetype_token);
				getline(name_quality_stream, name_quality_token, ';');
				type_name = name_quality_token;
				if (getline(name_quality_stream, name_quality_token, ';'))
					type_quality = atof(name_quality_token.c_str() + 2);
				_accepted_types[type_name] = type_quality;
			}

		} else if (temp_token == "Cookie:") {
			//to do cookies
		} else if (temp_token == "Host:") {
			int	host_start;
			int	host_end;
			int	delim_position;
			int	port_end;

			//extracting host and port from 		Host: localhost:8080
			host_start = temp_line.find_first_of(':') + 2;
			delim_position = temp_line.find(':', temp_line.find_first_of(':') + 1);

			if (delim_position == std::string::npos) { //Not tested
			//host without port
				host_end = temp_line.find_last_not_of(' ') + 1; //		Future check required: newline included or not to remove +1
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
		}
		temp_line.clear();
		temp_token.clear();
    }

	return true;
}

Request::Request() {
	_full_request = "";
	_file_path = "";
	_method = "";
	_http_version = "";
	_host = "localhost";
	_port = 80;
	_content_type = "";
}

Request::Request(std::string request) {
	_full_request = request;
	_file_path = "";
	_method = "";
	_http_version = "";
	_host = "localhost";
	_port = 80;
	_content_type = "";
	parseRequest();
}

Request::Request(const Request &obj) {
	_full_request = obj._full_request;
	_file_path = obj._file_path;
	_method = obj._method;
	_http_version = obj._http_version;
	_host = obj._host;
	_port = obj._port;
	_content_type = obj._content_type;
	_accepted_types = obj._accepted_types;
}

Request::~Request() {

}

void Request::setRequest(std::string request) {
	_full_request = request;

	parseRequest();
}

