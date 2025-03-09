#ifndef PARSER_HPP
# define PARSER_HPP
# include "HttpConfig.hpp"
# include <iostream>
# include <fstream>


/*

				TO DO
				
*/

class Parser {
	private:
		HttpConfig	*config;
		void ParseBlock(std::string block_name);
		std::ifstream	file;
		
	public:
		Parser();
		Parser(std::string file);
		Parser(const Parser &obj);
		Parser &operator=(const Parser &obj);
		~Parser();

		HttpConfig	*Parse();
};

#endif