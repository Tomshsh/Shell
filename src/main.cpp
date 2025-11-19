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
#include <fstream>

namespace fs = std::filesystem;

void loadHistoryLocal()
{
	char *histpath = getenv("HISTFILE");
	std::ifstream histfile(histpath);
	std::string line;
	while (getline(histfile, line))
		add_history(line.data());
}

void saveHistoryFile()
{
	char *histpath = getenv("HISTFILE");
	std::fstream histfile(histpath, std::ios::in | std::ios::out);
	std::string line;
	int hist_len = history_get_history_state()->length;
	int i = 0;
	for (; getline(histfile, line); i++)
		if (i < hist_len)
			continue;

	histfile.clear();
	histfile.seekp(0, std::ios::end);
	for (; i < hist_len; i++)
		histfile << history_get(history_base + i)->line << '\n';
}

int main()
{
	// Flush after every cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	std::atexit(saveHistoryFile);
	std::set_terminate([](){
		saveHistoryFile();
		std::abort();
	});

	loadHistoryLocal();

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
