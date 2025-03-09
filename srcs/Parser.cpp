#include "Parser.hpp"

Parser::Parser() {
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

}

HttpConfig *Parser::Parse() {
	std::string line;
	std::vector<std::string> tokens;

	file.open("config.conf");
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file" << std::endl;
		return (NULL);
	}

	
	return (new HttpConfig(*config));
}