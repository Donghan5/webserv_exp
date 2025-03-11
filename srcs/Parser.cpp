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

*/

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

		if ()
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
