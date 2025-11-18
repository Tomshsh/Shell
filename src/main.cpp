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

namespace fs = std::filesystem;

int main()
{
	// Flush after every cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	using_history();

	while (true)
	{
		rl_attempted_completion_function = Parser::character_name_completion;

		std::string input = readline("$ ");

		input += '\0';

		if (input.size() < 2)
			continue;

		add_history(input.data());

		Parser parser(input);
		parser.runCommand();
	}
}
