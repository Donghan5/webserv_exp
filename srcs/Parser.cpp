#include "Parser.hpp"

Parser::Parser() {
	_config = new HttpConfig();
}

Parser::Parser(STR file) {
	_config = new HttpConfig();

	_filepath = file;
}

Parser::Parser(const Parser &obj) {
	_config = obj._config;
}

Parser &Parser::operator=(const Parser &obj) {
	(void)obj;
	return (*this);
}

Parser::~Parser() {
	// if (_config)
	// 	delete _config;
}

int Parser::verifyPort(std::string port_str) {
	std::stringstream ss(port_str);
	int port;

	if (!(ss >> port) || port < 0 || port > 65335) {
		return -1;
	}
	return port;
}

bool Parser::verifyAutoIndex(std::string autoindex_str) {
	bool autoindex = false;
	if (autoindex_str == "on") {
		autoindex = true;
	}
	else if (autoindex_str == "off") {
		autoindex = false;
	}
	return autoindex;
}

/* default limit */
int Parser::veriftEventWorkerConnections(std::string event_worker_connections_str) {
	std::stringstream ss(event_worker_connections_str);
	int value;

	for (size_t i = 0; i < event_worker_connections_str.length(); i++) {
		if (!std::isdigit(event_worker_connections_str[i])) {
			return -1;
		}
	}

	if (!(ss >> value) || value <= 0) {
		return -1;
	}

	return value;
}

/* max value 1048576 */
long long Parser::verifyClientMaxBodySize(std::string client_max_body_size_str) {
	std::stringstream ss(client_max_body_size_str);
	long long value;
	std::string unit;

	if (!(ss >> value)) {
		return -1;
	}
	ss >> unit;

	if (value < 0) {
		return -1;
	}

	if (unit.empty() || unit == "b" || unit == "B") { // default byte
		value *= 1;
	}
	else if (unit == "k" || unit == "K") {
		value *= 1024;
	}
	else if (unit == "m" || unit == "M") {
		value *= 1024 * 1024;
	}
	else if (unit == "g" || unit == "G") {
		value *= 1024 * 1024 * 1024;
	}

	if (value > LLONG_MAX) {
		return -1;
	}

	return value;
}

VECTOR<STR>	split(STR string, char delim, bool use_whitespaces_delim) {
	VECTOR<STR>	result;
	STR					temp_line;
	std::istringstream	string_stream (string);

	// Split by any whitespace (>> skips at the beginning and then stops at any whitespace by default)
	if (use_whitespaces_delim) {
        std::string token;

        while (string_stream >> token) {
            result.push_back(token);
        }
		return result;
	}

	while (getline(string_stream >> std::ws, temp_line, delim)) {
		result.push_back(temp_line);
	}

	return result;
}

bool	isDirectiveOk(STR line, int start, int end) {
	VECTOR<STR>	tokens;
	STR			trimmed_line;

	trimmed_line = line.substr(start, end - start);
	// std::cerr << "TRIMMED |" << trimmed_line << "|\n";
	tokens = split(trimmed_line, ' ', 1);
	if (tokens.size() < 2)
		return false;
	return true;
}

bool	isBlockOk(STR line, int start, int end) {
	VECTOR<STR>	tokens;
	STR			trimmed_line;
	STR			block_name;

	trimmed_line = line.substr(start, end - start + 1);

	block_name = trimmed_line.substr(0, trimmed_line.find('{'));

	if (block_name == "")
		return false;

	//checking text before block
	tokens = split(block_name, ' ', 1);
	if (tokens.size() != 1 && tokens.size() != 2)
		return false;

	if (tokens[0] != "events" && tokens[0] != "http" && tokens[0] != "server" && tokens[0] != "location")
		return false;

	if (tokens[0] == "location" && tokens.size() != 2)
		return false;

	if (tokens[0] != "location" && tokens.size() != 1)
		return false;

	return true;
}

bool	isBlockEndOk(STR line, int start) {
	std::istringstream	string_stream;
	STR					trimmed_line;
	STR					no_ws;

	trimmed_line = line.substr(start);

	string_stream.str(trimmed_line);
	string_stream >> no_ws;
	if (no_ws[0] != '}')
		return false;
	return true;
}

ElemType	Parser::DetectNextType(STR line, int position, int &block_size) {
	int	semicol = line.find(';', position);
	int	open_brace = line.find('{', position);
	int	close_brace = line.find('}', position);
	static int depth;

	block_size = 0;

	VECTOR<STR> last_char_check = split(line.c_str() + position, ' ', 1);
	if (last_char_check.size() == 1 &&
		last_char_check[0].size() == 1 &&
		last_char_check[0][0] == '}' &&
		depth != 1) {
			return BAD_TYPE;
	}

	if (semicol != CHAR_NOT_FOUND) {
		if (open_brace != CHAR_NOT_FOUND && close_brace != CHAR_NOT_FOUND) {
			if (semicol < open_brace && semicol < close_brace) {
				if (isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else if (open_brace != CHAR_NOT_FOUND) {
			if (semicol < open_brace) {
				if (isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else if (close_brace != CHAR_NOT_FOUND) {
			if (semicol < close_brace) {
				if (isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else {	//no open and no close braces
			if (isDirectiveOk(line, position, semicol)) {
				block_size = semicol - position + 1;
				return DIRECTIVE;
			}
			else
				return BAD_TYPE;
		}
	}
	if (open_brace != CHAR_NOT_FOUND && close_brace != CHAR_NOT_FOUND) {
		if (close_brace < open_brace && depth <= 0)
			return BAD_TYPE;
		if (close_brace < open_brace) {
			if (isBlockEndOk(line, position)) {
				block_size = close_brace - position + 1;
				depth--;
				return BLOCK_END;
			}
		} else if (isBlockOk(line, position, close_brace)) {
			block_size = open_brace - position + 1;
			depth++;
			return BLOCK;
		}
	} else if (close_brace != CHAR_NOT_FOUND) {
		if (depth <= 0)
			return BAD_TYPE;
		if (isBlockEndOk(line, position)) {
			block_size = close_brace - position + 1;
			depth--;
			return BLOCK_END;
		}
	}
	// else if (open_brace != CHAR_NOT_FOUND) {
	// 	if (isBlockOk(line, position, close_brace)) {
	// 		block_size = open_brace - position + 1;
	// 		depth++;
	// 		return BLOCK;
	// 	}
	// }

	return BAD_TYPE;
}

bool	Parser::ValidateConfig(STR full_config) {
	int	size = 1;

	for (size_t i = 0; i < full_config.size(); i += size)
	{
		switch (DetectNextType(full_config, i, size))
		{
		case DIRECTIVE:
			// std::cerr << (full_config.c_str() + i) << "\nTYPE: DIRECTIVE\n\n\n\n";
			break;
		case BLOCK:
			// std::cerr << (full_config.c_str() + i) << "\n\n TYPE: BLOCK\n\n";
			break;
		case BLOCK_END:
			// std::cerr << (full_config.c_str() + i) << "\n\n TYPE: BLOCK_END\n\n";
			break;
		default:
			// std::cerr << (full_config.c_str() + i) << "\n\n TYPE: BAD\n\n";
			size = -1;
			break;
		}
		if (size == -1)
			break;
	}
	if (size == -1)
		return false;
	return true;
}

#define VAR_NAME(var) #var

bool FillDirective(AConfigBase* block, STR line, int position) {
	VECTOR<STR> tokens;
	STR 		trimmed_line;

	int semicol = line.find(';', position);
	trimmed_line = line.substr(position, semicol - position);
	tokens = split(trimmed_line, ' ', 1);

	if (HttpConfig* httpConf = dynamic_cast<HttpConfig*>(block)) {
		if (tokens[0] == "user") {
			httpConf->_global_user = tokens[1];
		} else if (tokens[0] == "worker_process") {
			httpConf->_global_worker_process = tokens[1];
		} else if (tokens[0] == "DEBUG_log") {
			httpConf->_global_error_log = tokens[1];
		} else if (tokens[0] == "pid") {
			httpConf->_global_pid = tokens[1];
		} else if (tokens[0] == "worker_connections") {
			httpConf->_event_worker_connections = Parser::veriftEventWorkerConnections(tokens[1]);  // C++98 int conversion
			if (httpConf->_event_worker_connections == -1) {
				std::cerr << "Invalid worker_connections value";
				return false;
			}
		} else if (tokens[0] == "use") {
			httpConf->_event_use = tokens[1];
		} else if (tokens[0] == "log_format") {
			httpConf->_log_format = tokens[1];
		} else if (tokens[0] == "access_log") {
			httpConf->_access_log = tokens[1];
		} else if (tokens[0] == "sendfile") {
			httpConf->_sendfile = tokens[1];
		} else if (tokens[0] == "keepalive_timeout") {
			httpConf->_keepalive_timeout = tokens[1];
		} else if (tokens[0] == "add_header") {
			httpConf->_add_header = tokens[1];
		} else if (tokens[0] == "client_max_body_size") {
			httpConf->_client_max_body_size = Parser::verifyClientMaxBodySize(tokens[1]);  // C++98 long long
			if (httpConf->_client_max_body_size == -1) {
				std::cerr << "Invalid client_max_body_size value" << std::endl;
				return false;
			}
		} else if (tokens[0] == "root") {
			httpConf->_root = tokens[1];
		} else if (tokens[0] == "index") {
			for (size_t j = 1; j < tokens.size(); j++) {
				httpConf->_index.push_back(tokens[j]);
			}
		} else if (tokens[0] == "DEBUG_page") {
			int code = atoi(tokens[1].c_str());
			httpConf->_error_pages[code] = tokens[2];  // Assumes "DEBUG_page 404 /404.html"
		} else {
			std::cerr << "DEBUG CHECKFillDirective HttpConfig extra type " << tokens[0] << "\n";
			return false;
		}
        return true;
    }
	else if (ServerConfig* serverConf = dynamic_cast<ServerConfig*>(block)) {
		if (tokens[0] == "add_header") {
			serverConf->_add_header = tokens[1];
		} else if (tokens[0] == "listen") {
			serverConf->_listen_port = Parser::verifyPort(tokens[1]);
			if (serverConf->_listen_port == -1) {
				std::cerr << "Invalid port value" << std::endl;
				return false;
			}
		} else if (tokens[0] == "server_name") {
			for (size_t j = 1; j < tokens.size(); j++) {
				serverConf->_server_name.push_back(tokens[j]);
			}

		// } else if (tokens[0] == "location") {
		//     serverConf->_location = tokens[1];
		} else if (tokens[0] == "try_files") {
			serverConf->_try_files = tokens[1];
		} else if (tokens[0] == "root") {
			serverConf->_root = tokens[1];
		} else if (tokens[0] == "index") {
			for (size_t j = 1; j < tokens.size(); j++) {
				serverConf->_index.push_back(tokens[j]);
			}
		} else if (tokens[0] == "DEBUG_page") {
			int code = atoi(tokens[1].c_str());
			serverConf->_error_pages[code] = tokens[2];
		} else if (tokens[0] == "client_max_body_size") {
			serverConf->_client_max_body_size = Parser::verifyClientMaxBodySize(tokens[1]);
			if (serverConf->_client_max_body_size == -1) {
				std::cerr << "Invalid client_max_body_size value" << std::endl;
				return false;
			}
		} else {

	std::cerr << "DEBUG CHECKFillDirective ServerConfig extra type " << tokens[0] << "\n";
			return false;
		}
        return true;
    }
    else if (LocationConfig* locConf = dynamic_cast<LocationConfig*>(block)) {
		if (tokens[0] == "path") {
			locConf->_path = tokens[1];
		} else if (tokens[0] == "add_header") {
			locConf->_add_header = tokens[1];
		} else if (tokens[0] == "return") {
			locConf->_return = tokens[1];
		} else if (tokens[0] == "allow") {
			locConf->_allow = tokens[1];
		} else if (tokens[0] == "deny") {
			locConf->_deny = tokens[1];
		} else if (tokens[0] == "alias") {
			locConf->_alias = tokens[1];
		} else if (tokens[0] == "try_files") {
			locConf->_try_files = tokens[1];
		} else if (tokens[0] == "root") {
			locConf->_root = tokens[1];
		} else if (tokens[0] == "client_max_body_size") {
			locConf->_client_max_body_size = Parser::verifyClientMaxBodySize(tokens[1]);
			if (locConf->_client_max_body_size == -1) {
				std::cerr << "Invalid client_max_body_size value" << std::endl;
				return false;
			}
		} else if (tokens[0] == "autoindex") {
			// locConf->_autoindex = (tokens[1] == "on");  // Simple bool conversion
			locConf->_autoindex = Parser::verifyAutoIndex(tokens[1]);
		} else if (tokens[0] == "index") {
			for (size_t j = 1; j < tokens.size(); j++) {
				locConf->_index.push_back(tokens[j]);
			}
		} else if (tokens[0] == "DEBUG_page") {
            int code = atoi(tokens[1].c_str());
            locConf->_error_pages[code] = tokens[2];
        } else if (tokens[0] == "allowed_methods") {
			for (size_t j = 1; j < tokens.size(); j++) {
				if (tokens[j] != "GET" && tokens[j] != "POST" && tokens[j] != "DELETE")
					return false;
				locConf->_allowed_methods[tokens[j]] = true;
			}
        } else {

	std::cerr << "DEBUG CHECKFillDirective LocationConfig extra type "  << tokens[0] << "\n";
			return false;
		}
        return true;
    }
	std::cerr << "DEBUG CHECKFillDirective Unknown block type" << "\n";

    return false;  // Unknown block type
}

AConfigBase	*CreateBlock(STR line, int start) {
	AConfigBase *block;
	VECTOR<STR>	tokens;
	STR			trimmed_line;
	STR			block_name;
	STR			block_body;
	int	close_brace = line.find('}', start);

	trimmed_line = line.substr(start, close_brace - start);
	// std::cerr << "DEBUG AConfigBase	*CreateBlock TRIMMED |" << trimmed_line << "|\n";
	// std::cerr << "DEBUG AConfigBase	*CreateBlock start |" << start << "|\n";
	// std::cerr << "DEBUG AConfigBase	*CreateBlock close_brace |" << close_brace << "|\n";
	// std::cerr << "DEBUG AConfigBase	*CreateBlock line |" << line << "|\n";

	block_name = trimmed_line.substr(0, trimmed_line.find('{'));
	tokens = split(block_name, ' ', 1);

	block = NULL;
	// std::cerr << "DEBUG AConfigBase	*CreateBlock\n";
	if (tokens[0] == "events" || tokens[0] == "http") {
		HttpConfig *conf = new HttpConfig();
		block = conf;
		std::cerr << "DEBUG AConfigBase	*CreateBlock CREATED http\n";

	} else if (tokens[0] == "server") {
		ServerConfig *conf = new ServerConfig();
		block = conf;
		std::cerr << "DEBUG AConfigBase	*CreateBlock CREATED server\n";
	} else if (tokens[0] == "location") {
		LocationConfig *conf = new LocationConfig();
		conf->_path = tokens[1];
		block = conf;
		std::cerr << "DEBUG AConfigBase	*CreateBlock CREATED location\n";
	}
	std::cerr << "DEBUG AConfigBase	*CreateBlock2\n";

	return block;
}

bool	check_location_path_duplicate(STR new_path, MAP<STR, LocationConfig*> locs) {
	//if path is new trying to access it may throw exception
	try
	{
		LocationConfig *test = locs[new_path];
		if (!test)
			return true;
	}
	catch(const std::exception& e)
	{
		return true;
	}
	return false;
}

AConfigBase	*AddBlock(AConfigBase *prev_block, STR line, int start) {
	AConfigBase	*child = NULL;
	ConfigBlock parent_type = ERROR;

	// std::cerr << "DEBUG AConfigBase	*AddBlock entry\n";
	// std::cerr << "DEBUG AConfigBase	*AddBlock start " << start << "\n";
	// std::cerr << "DEBUG AConfigBase	*AddBlock line " << line.c_str() + start << "\n";
	// std::cerr << "DEBUG AConfigBase	*AddBlock entry\n";
	// std::cerr << "DEBUG AConfigBase	*AddBlock entry\n";

	parent_type = prev_block->_identify(prev_block);
	if (parent_type == HTTP) {
		HttpConfig* httpConf = dynamic_cast<HttpConfig*>(prev_block);
		ServerConfig* serverConf;

		child = CreateBlock(line, start);
		if (!child || !httpConf || httpConf->_identify(child) != SERVER) {
			if (child)
				child->_self_destruct();
			std::cerr << "DEBUG AConfigBase	*AddBlock HTTP\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock !httpConf " << !httpConf << "\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock !child " << !child << "\n";

			// std::cerr << "DEBUG AConfigBase	*AddBlock != SERVER : " << (httpConf->_identify(child) != SERVER) << "\n";
			return NULL;
		}
		serverConf = dynamic_cast<ServerConfig*>(child);
		httpConf->_servers.push_back(serverConf);
		// serverConf->back_ref = httpConf;
		child->back_ref = prev_block;
	} else if (parent_type == SERVER) {
		ServerConfig* serverConf = dynamic_cast<ServerConfig*>(prev_block);
		LocationConfig* location;

		child = CreateBlock(line, start);
		if (!child || !serverConf || serverConf->_identify(child) != LOCATION) {
			if (child)
				child->_self_destruct();
			std::cerr << "DEBUG AConfigBase	*AddBlock SERVER\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock !serverConf" << !serverConf << "\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock !child" << !child << "\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock != LOCATION" << (serverConf->_identify(child) != LOCATION) << "\n";

			return NULL;
		}
		location = dynamic_cast<LocationConfig*>(child);

		if (!check_location_path_duplicate(location->_path, serverConf->_locations)) {
			child->_self_destruct();

			std::cerr << "DEBUG AConfigBase	*AddBlock LOCATION DUPLICATE " << location->_path << "\n";

			return NULL;
		}

		serverConf->_locations[location->_path] = location;
		// location->back_ref = serverConf;
		child->back_ref = prev_block;
	} else if (parent_type == LOCATION) {
		LocationConfig* locParent = dynamic_cast<LocationConfig*>(prev_block);
		LocationConfig* locChild;

		child = CreateBlock(line, start);
		if (!child || !locParent || locParent->_identify(child) != LOCATION) {
			if (child)
				child->_self_destruct();
			std::cerr << "DEBUG AConfigBase	*AddBlock LOCATION\n";
			std::cerr << "DEBUG AConfigBase	*AddBlock !child" << !child << "\n";
			return NULL;
		}
		locChild = dynamic_cast<LocationConfig*>(child);
		locParent->_locations[locChild->_path] = locChild;
		// locChild->back_ref = locParent;
		child->back_ref = prev_block;
	} else {
		std::cerr << "DEBUG: AddBlockNULL TYpe" << parent_type << std::endl;
		return NULL;
	}
	std::cerr << "DEBUG AConfigBase	*AddBlock exit\n";

	return child;
}

bool	minimum_value_check(HttpConfig *conf) {
	if (conf->_servers.empty()) {
		std::cerr << "ERROR no servers found\n";
		return false;
	}

	for (size_t i = 0; i < conf->_servers.size(); i++)
	{
		//if it's the only block - defaults to 80
		if (conf->_servers[i]->_listen_port == -1 && conf->_servers.size() == 1) {
			conf->_servers[i]->_listen_port = 80;
		}
		if (conf->_servers[i]->_listen_port == -1) {
	std::cerr << "ERROR server without port found\n";
			return false;
		}
		if (conf->_servers[i]->_locations.empty()) {
	std::cerr << "ERROR server without location found\n";
			return false;
		}
	}
	return true;
}

HttpConfig *Parser::Parse() {
	STR			line;
	VECTOR<STR>	tokens;
	STR			full_config;

	if (_filepath == "")
		_file.open("config.conf");
	else
		_file.open(_filepath.c_str());
	if (!_file.is_open()) {
std::cerr << "DEBUG: Could not open file" << std::endl;
		return (NULL);
	}

	STR temp_line;
	while (getline(_file, temp_line)) {
		full_config += temp_line;
    }
	_file.close();

	if (!ValidateConfig(full_config)) {
		std::cerr << "Config is not correct!\n";
		return NULL;
	}

	int	size = 1;

	HttpConfig *base = new HttpConfig;
	base->back_ref = NULL;
	AConfigBase	*currentBlock = base;

	bool skipped_block1 = -1;
	bool skipped_block2 = -1;
	MAP<int, int>	directives_per_block;
	int depth = 0;
	for (size_t i = 0; i < full_config.size(); i += size)
	{
		switch (DetectNextType(full_config, i, size))
		{
		case DIRECTIVE:
			directives_per_block[depth]++;
			std::cerr << "DIRECTIVE (" << (full_config.substr(i, 50)) <<  ")\n";
			if (!FillDirective(currentBlock, full_config, i)) {
				base->_self_destruct();

				std::cerr << "ERROR CHECKFillDirective\n";
				return NULL;
			}
			std::cerr << "--DIRECTIVE\n";
			break;
		case BLOCK:
			depth++;
			directives_per_block[depth] = 0;
			std::cerr << "BLOCK (" << (full_config.substr(i, 50)) <<  ")\n";
			//if event/http, we already have default one
			if (!strncmp(full_config.c_str() + i, "http", 4) || !strncmp(full_config.c_str() + i, "events", 6)) {
				skipped_block1 = depth;
				std::cerr << "--(skip)BLOCK\n";
				continue;
			}

			currentBlock = AddBlock(currentBlock, full_config, i);
			if (!currentBlock) {
				base->_self_destruct();

				std::cerr << "ERROR CHECKAddBlock\n";
				return NULL;
			}
			std::cerr << "--BLOCK\n";
			break;
		case BLOCK_END:
			std::cerr << "BLOCK_END (" << (full_config.substr(i, 50)) <<  ")\n";
			if (skipped_block1 == depth)
				skipped_block1 = -1;
			else if (skipped_block2 == depth)
				skipped_block2 = -1;
			else
				currentBlock = currentBlock->back_ref;
			if (directives_per_block[depth] == 0) {
				//ERROR: block doesn't have it's own directives!
				base->_self_destruct();

				std::cerr << "ERROR: block doesn't have it's own directives!\n";
				return NULL;
			}
			depth--;
			std::cerr << "--BLOCK_END\n";
			break;
		default:
			size = -1;
			break;
		}
		if (size == -1)
			break;
	}

	if (base->_identify(currentBlock) != HTTP || currentBlock != base || depth != 0) {
		base->_self_destruct();
		return NULL;
	}

	if (!minimum_value_check(base)) {
		base->_self_destruct();
		return NULL;
	}

	return (base);
}
