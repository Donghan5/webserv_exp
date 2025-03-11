#ifndef PARSER_HPP
# define PARSER_HPP
# include "HttpConfig.hpp"
# include "Logger.hpp"
# include <iostream>
# include <fstream>


/*

				TO DO

*/

class Parser {
	private:
		HttpConfig	*config;
		void ParseBlock(std::string block_name);
		std::string _file_name;
		std::ifstream	file;

	public:
		Parser(std::string file_name);
		Parser(std::string file);
		Parser(const Parser &obj);
		Parser &operator=(const Parser &obj);
		~Parser();

		HttpConfig	*Parse(std::string file_name);
};

#endif
