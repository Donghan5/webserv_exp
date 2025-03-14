#include "Parser.hpp"

Parser::Parser(std::string file_name): _file_name(file_name) {
	config = new HttpConfig();
}

Parser::Parser(std::string file) {
	config = new HttpConfig();
}

Parser::Parser(const Parser &obj) {
	config = obj.config;
}

Parser &Parser::operator=(const Parser &obj) {
	return (*this);
}

Parser::~Parser() {
	if (config)
		delete config;
}

/*

	TO DO
	벡터를 제외하고는, 그냥 토큰화를 해서 타입 변환이 필요하면 타입 변환을 해서 setter 부분에서 처리를 할거야.
	이게 내가 생각하기에는 자주 쓰는 값들을 뽑아 쓰기도 편하고, 확실하게 저장을 할 것 같음
	맵으로 저장을 하고, 자주 쓰는 인덱스들을 setter를 활용해서 변환 저장, getter로 가져올 수 있게
	일반적인 파싱 파트에서 자주 쓰는 인덱스들을 분류를 해서 setter로 세팅을 해야하나????
*/

template < typename T >
void Parser::parseKeyValue(std::string line, T &config) {
	std::vector<std::string> tokens = Utils::split(line, ' ');

	if (tokens.size() < 2) {
		std::cerr << "Invalid format" << std::endl;
		return;
	}
	std::string key = tokens[0];
	config.setData(key, tokens[1]);
}

template <>
void Parser::parseKeyValue(std::string line, HttpConfig &config) {
	std::vector<std::string> tokens = Utils::split(line, ' ');

	if (tokens.size() < 2) {
		std::cerr << "Invalid format" << std::endl;
		return;
	}
	std::string key = tokens[0];
	config.setData(key, tokens[1]);
}

template <>
void Parser::parseKeyValue(std::string line, ServerConfig &config) {
	std::vector<std::string> tokens = Utils::split(line, ' ');

	if (tokens.size() < 2) {
		std::cerr << "Invalid format" << std::endl;
		return;
	}
	std::string key = tokens[0];
	std::string value = tokens[1];
	if (key == "server_name") {
		for (size_t i = 2; i < tokens.size(); i++) {
			value += " " + tokens[i];
		}
	}
	config.setData(key, value)
}

template <>
void Parser::parseKeyValue(std::string line, LocationConfig &config) {
	std::vector<std::string> tokens = Utils::split(line, ' ');

	if (tokens.size() < 2) {
		std::cerr << "Invalid format" << std::endl;
		return;
	}
	config.setPath(tokens[1]); // Set path for location block
    config.setData("path", tokens[1]);

	std::string key = tokens[0];
	std::string value = tokens[1];
	if (key == "server_name") {
		for (size_t i = 2; i < tokens.size(); i++) {
			value += " " + tokens[i];
		}
	}
	if (key == "add_header") config.setAddHeader(value);
	else if (key == "proxy_pass") config.setProxyPass(value);
	else if (key == "allow") config.setAllow(value);
	else if (key == "deny") consif.setDeny(value);
	config.setData(key, value);
}

void	Parser::ParseBlock(std::string block_name) {
	std::string line;
	int bracket_count = 1;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {  // line empty or comment
			continue ;
		}

		if (line == "}") {
			bracket_count--;
			if (bracket_count == 0) {
				return;
			}
			continue;
		}

		if (line.find("{") != std::string::npos) {
			bracket_count++;
		}

		if (block_name == "location") {
			// location store
		}
		else if (block_name == "server") {
			// server store
		}
		else if (block_name == "http" || block_name == "event") {
			// http and event
		}
	}
}

HttpConfig *Parser::Parse(std::string file_name) {
	std::string line;
	std::vector<std::string> tokens;

	file.open(file_name.c_str());
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file" << std::endl;
		return (NULL);
	}
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {  // line empty or comment
			continue ;
		}

		if (line.find("server {") != std::string::npos) {
			ParseBlock("server");
		}
		else if (line.find("location {") != std::string::npos) {
			ParseBlock("location");
		}
		else if (line.find("http {") != std::string::npos) {
			ParseBlock("http");
		}
		else {
			// basic key value store ?
		}
	}

	return (new HttpConfig(*config));
}
