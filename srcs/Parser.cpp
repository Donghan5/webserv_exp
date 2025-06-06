#include "Parser.hpp"
#include "Logger.hpp"

Parser::Parser() {
	_config = new HttpConfig();
}

Parser::Parser(STR file) {
	_config = new HttpConfig();

	_filepath = file;
}

Parser::Parser(const Parser &obj) {
	_config = obj._config;
	_filepath = obj._filepath;
}

Parser &Parser::operator=(const Parser &obj) {
	(void)obj;
	return (*this);
}

Parser::~Parser() {
	if (_config)
		delete _config;
}


ElemType	Parser::DetectNextType(STR line, int position, int &block_size) {
	int	semicol = line.find(';', position);
	int	open_brace = line.find('{', position);
	int	close_brace = line.find('}', position);
	static int depth;

	block_size = 0;

	VECTOR<STR> last_char_check = Utils::split(line.c_str() + position, ' ', 1);
	if (last_char_check.size() == 1 &&
		last_char_check[0].size() == 1 &&
		last_char_check[0][0] == '}' &&
		depth != 1) {
			return BAD_TYPE;
	}

	if (semicol != CHAR_NOT_FOUND) {
		if (open_brace != CHAR_NOT_FOUND && close_brace != CHAR_NOT_FOUND) {
			if (semicol < open_brace && semicol < close_brace) {
				if (ParserUtils::isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else if (open_brace != CHAR_NOT_FOUND) {
			if (semicol < open_brace) {
				if (ParserUtils::isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else if (close_brace != CHAR_NOT_FOUND) {
			if (semicol < close_brace) {
				if (ParserUtils::isDirectiveOk(line, position, semicol)) {
					block_size = semicol - position + 1;
					return DIRECTIVE;
				}
				else
					return BAD_TYPE;
			}
		} else {	//no open and no close braces
			if (ParserUtils::isDirectiveOk(line, position, semicol)) {
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
			if (ParserUtils::isBlockEndOk(line, position)) {
				block_size = close_brace - position + 1;
				depth--;
				return BLOCK_END;
			}
		} else if (ParserUtils::isBlockOk(line, position, close_brace)) {
			block_size = open_brace - position + 1;
			depth++;
			return BLOCK;
		}
	} else if (close_brace != CHAR_NOT_FOUND) {
		if (depth <= 0)
			return BAD_TYPE;
		if (ParserUtils::isBlockEndOk(line, position)) {
			block_size = close_brace - position + 1;
			depth--;
			return BLOCK_END;
		}
	}
	// else if (open_brace != CHAR_NOT_FOUND) {
	// 	if (ParserUtils::isBlockOk(line, position, close_brace)) {
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



AConfigBase	*CreateBlock(STR line, int start) {
	AConfigBase *block;
	VECTOR<STR>	tokens;
	STR			trimmed_line;
	STR			block_name;
	STR			block_body;
	int	close_brace = line.find('}', start);

	trimmed_line = line.substr(start, close_brace - start);
	block_name = trimmed_line.substr(0, trimmed_line.find('{'));
	tokens = Utils::split(block_name, ' ', 1);

	block = NULL;
	if (tokens[0] == "events" || tokens[0] == "http") {
		HttpConfig *conf = new HttpConfig();
		block = conf;
		Logger::log(Logger::DEBUG, "AConfigBase *CreateBlock CREATED http");

	} else if (tokens[0] == "server") {
		ServerConfig *conf = new ServerConfig();
		block = conf;
		Logger::log(Logger::DEBUG, "AConfigBase *CreateBlock CREATED server");
	} else if (tokens[0] == "location") {
		LocationConfig *conf = new LocationConfig();
		conf->_path = tokens[1];
		block = conf;
		Logger::log(Logger::DEBUG, "AConfigBase *CreateBlock CREATED location");
	}
	Logger::log(Logger::DEBUG, "AConfigBase *CreateBlock CREATED block2");

	return block;
}

bool	check_location_path_duplicate(STR new_path, MAP<STR, LocationConfig*> locs) {
	try
	{
		MAP<STR, LocationConfig*> loc_loc = locs;
		while (loc_loc.size() > 0) {
			MAP<STR, LocationConfig*>::iterator it = loc_loc.begin();
			if (it->first == new_path)
				return false;
			if (it->second->_locations.size() > 0) {
				loc_loc.insert(it->second->_locations.begin(), it->second->_locations.end());
			}
			loc_loc.erase(it);
		}
	}
	catch(const std::exception& e)
	{
		return false;
	}
	return true;
}

AConfigBase	*AddBlock(AConfigBase *prev_block, STR line, int start) {
	AConfigBase	*child = NULL;
	ConfigBlock parent_type = ERROR;

	parent_type = prev_block->_identify(prev_block);
	if (parent_type == HTTP) {
		HttpConfig* httpConf = dynamic_cast<HttpConfig*>(prev_block);
		ServerConfig* serverConf;

		child = CreateBlock(line, start);
		if (!child || !httpConf || httpConf->_identify(child) != SERVER) {
			if (child)
				child->_self_destruct();
			return NULL;
		}
		serverConf = dynamic_cast<ServerConfig*>(child);
		httpConf->_servers.push_back(serverConf);
		child->back_ref = prev_block;
	} else if (parent_type == SERVER) {
		ServerConfig* serverConf = dynamic_cast<ServerConfig*>(prev_block);
		LocationConfig* location;

		child = CreateBlock(line, start);
		if (!child || !serverConf || serverConf->_identify(child) != LOCATION) {
			if (child)
				child->_self_destruct();
			return NULL;
		}
		location = dynamic_cast<LocationConfig*>(child);

		if (!check_location_path_duplicate(location->_path, serverConf->_locations)) {
			child->_self_destruct();

			Logger::cerrlog(Logger::ERROR, "AConfigBase *AddBlock LOCATION DUPLICATE " + location->_path);

			return NULL;
		}

		serverConf->_locations[location->_path] = location;
		// location->back_ref = serverConf;
		child->back_ref = prev_block;
	} else if (parent_type == LOCATION) {
		LocationConfig* locParent = dynamic_cast<LocationConfig*>(prev_block);
		LocationConfig* locChild;

		// this is changed, adding verication dup server name
		ServerConfig* serverConf = NULL;

		child = CreateBlock(line, start);
		if (!child || !locParent || locParent->_identify(child) != LOCATION) {
			if (child)
				child->_self_destruct();
			return NULL;
		}
		locChild = dynamic_cast<LocationConfig*>(child);

		// find server
		AConfigBase* temp = prev_block;
		while (temp && temp->_identify(temp) != HTTP) {
			if (temp->_identify(temp) == SERVER) {
				serverConf = dynamic_cast<ServerConfig*>(temp);
				break;
			}
			temp = temp->back_ref;
		}

		if (!serverConf || serverConf->_identify(serverConf) != SERVER) {
			if (child)
				child->_self_destruct();
			std::cerr << "DEBUG AConfigBase	*AddBlock !serverConf" << !serverConf << "\n";
			return NULL;
		}

		// new thing, check dup server name
		if (!check_location_path_duplicate(locChild->_path, serverConf->_locations)) {
			child->_self_destruct();

			Logger::cerrlog(Logger::ERROR, "AConfigBase *AddBlock LOCATION DUPLICATE " + locChild->_path);

			return NULL;
		}

		locParent->_locations[locChild->_path] = locChild;
		// locChild->back_ref = locParent;
		child->back_ref = prev_block;
	} else {
		std::stringstream ss;
		ss << "AddBlockNULL Type " << static_cast<int>(parent_type);
		Logger::cerrlog(Logger::ERROR, ss.str());
		return NULL;
	}
	Logger::log(Logger::INFO, "AConfigBase *AddBlock exit");

	return child;
}

bool	minimum_value_check(HttpConfig *conf) {
	if (conf->_servers.empty()) {
		Logger::cerrlog(Logger::ERROR, "No servers found");
		return false;
	}

	for (size_t i = 0; i < conf->_servers.size(); i++)
	{
		//if it's the only block - defaults to 80
		if (conf->_servers[i]->_listen_port == -1 && conf->_servers.size() == 1) {
			conf->_servers[i]->_listen_port = 80;
		}
		if (conf->_servers[i]->_listen_port == -1) {
			Logger::cerrlog(Logger::ERROR, "Server without port found");
			return false;
		}
		if (conf->_servers[i]->_locations.empty()) {
			Logger::cerrlog(Logger::ERROR, "Server without location found");
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
		Logger::cerrlog(Logger::ERROR, "Could not open file " + _filepath);
		return (NULL);
	}

	STR temp_line;
	while (getline(_file, temp_line)) {
		full_config += temp_line;
    }
	_file.close();

	if (!ValidateConfig(full_config)) {
		Logger::cerrlog(Logger::ERROR, "Config is not correct!");
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
			Logger::log(Logger::DEBUG, "DIRECTIVE (" + full_config.substr(i, 50) + ")");
			if (!ParserFiller::FillDirective(currentBlock, full_config, i)) {
				base->_self_destruct();

				Logger::cerrlog(Logger::ERROR, "CHECKFillDirective");
				return NULL;
			}
			std::cerr << "--DIRECTIVE\n";
			break;
		case BLOCK:
			depth++;
			directives_per_block[depth] = 0;
			Logger::log(Logger::DEBUG, "BLOCK (" + full_config.substr(i, 50) + ")");
			//if event/http, we already have default one
			if (!strncmp(full_config.c_str() + i, "http", 4) || !strncmp(full_config.c_str() + i, "events", 6)) {
				skipped_block1 = depth;
				std::cerr << "--(skip)BLOCK\n";
				continue;
			}

			currentBlock = AddBlock(currentBlock, full_config, i);
			if (!currentBlock) {
				base->_self_destruct();

				Logger::log(Logger::ERROR, "CHECKAddBlock");
				return NULL;
			}
			std::cerr << "--BLOCK\n";
			break;
		case BLOCK_END:
			Logger::log(Logger::DEBUG, "BLOCK_END (" + full_config.substr(i, 50) + ")");
			if (skipped_block1 == depth)
				skipped_block1 = -1;
			else if (skipped_block2 == depth)
				skipped_block2 = -1;
			else
				currentBlock = currentBlock->back_ref;
			if (directives_per_block[depth] == 0) {
				//ERROR: block doesn't have it's own directives!
				base->_self_destruct();

				Logger::cerrlog(Logger::ERROR, "CHECKFillDirective block doesn't have it's own directives!");
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
