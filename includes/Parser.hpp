#ifndef PARSER_HPP
# define PARSER_HPP
# include "HttpConfig.hpp"
# include "Logger.hpp"
# include "Utils.hpp"
# include <iostream>
# include <fstream>


/*

	Main parsing class
		http: http and event
		server
		location

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

		template < typename T >
		void parseKeyValue(std::string line, T &config);

		template <>
		void parseKeyValue(std::string line, HttpConfig &config);

		template <>
		void parseKeyValue(std::string line, ServerConfig &config);

		template <>
		void parseKeyValue(std::string line, LocationConfig &config);

		HttpConfig	*Parse(std::string file_name);
};

#endif
