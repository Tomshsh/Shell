#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <Parser.hpp>
#include <readline/readline.h>

namespace fs = std::filesystem;

/////////////////////////////# DECLARATIONS #///////////////////////////////

const char *path_delimiter = []
{
#ifdef _WIN32
	return ";";
#else
	return ":";
#endif
}();

char *p_env = {};

/////////////////////////////# MAIN #///////////////////////////////

int main()
{
	// Flush after every cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	p_env = getenv("PATH");
	std::string env_p = p_env ? p_env : "";

	while (true)
	{
		rl_attempted_completion_function = Parser::character_name_completion;
		
		std::string input = readline("$ ");
		
		input += '\0';
		
		Parser parser(input);
		parser.runCommand();
	}
}
