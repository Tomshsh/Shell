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


namespace fs = std::filesystem;

/////////////////////////////# DECLARATIONS #///////////////////////////////

enum command_type
{
	BUILTIN
};

struct Command
{
	std::string command;
	command_type type;
	std::function<void(std::stringstream &)> cb;

	Command(std::string cmd, command_type cmd_type, std::function<void(std::stringstream &)> callback) : command(cmd), type(cmd_type), cb(std::move(callback)) {};
};

std::unordered_map<std::string, Command> commandMap;

const char *path_delimiter = []{
	#ifdef _WIN32
		return ";";
	#else
		return ":";
	#endif
}();

char *p_env = {};

/////////////////////////////# UTILS #///////////////////////////////

std::string TypeToStr(command_type cmd)
{
	switch (cmd)
	{
	case BUILTIN:
		return "builtin";

	default:
		break;
	}
}

std::vector<std::string> split(std::string str, std::string delim)
{
	std::vector<std::string> arr;
	char *d_str = str.data(), *d_delim = delim.data();
	char *tok = strtok(d_str, d_delim);

	while (tok != NULL)
	{
		arr.push_back(std::string(tok));
		tok = strtok(NULL, d_delim);
	}

	return arr;
}

/////////////////////////////# METHODS #///////////////////////////////

void handleExit(std::stringstream &ss)
{
	std::string code;
	ss >> code;
	int exit_code = 0;
	if (code.length() > 0)
	{
		if (!(code.at(0) == '0' || code.at(0) == 1))
		{
			printf("exit: invalid argument\n");
			return;
		}
		exit_code = code.at(0) - '0';
	}
	exit(exit_code);
}

void handleEcho(std::stringstream &ss)
{
	ss.seekg(5);
	std::cout << ss.rdbuf() << "\n";
}

void handleType(std::stringstream &ss)
{
	std::string com;
	ss >> com;

	std::string env_p = p_env ? p_env : "";

	auto it = commandMap.find(com);
	if (it != commandMap.end())
	{
		printf("%s is a shell %s\n", com.c_str(), TypeToStr(it->second.type).c_str());
		return;
	}
	else
	{
		std::vector<std::string> arr = split(env_p, path_delimiter);
		for (std::string str : arr)
		{
			const fs::path pth(str);
			if (!fs::exists(pth))
				continue;

			for (auto const &dir_entry : fs::recursive_directory_iterator(pth, fs::directory_options::skip_permission_denied))
			{
				if (com == dir_entry.path().filename().string())
				{
					if (access(dir_entry.path().c_str(), X_OK) != 0) continue;	// is not executable

					std::cout << com << " is " << dir_entry.path().string() << "\n";
					return;
				}
			}
		}
	}
	printf("%s: not found\n", com.c_str());
	return;
}

/////////////////////////////# MAIN #///////////////////////////////

void initCommandMap()
{
	commandMap = {
		{"exit", Command("exit", BUILTIN, handleExit)},
		{"echo", Command("echo", BUILTIN, handleEcho)},
		{"type", Command("type", BUILTIN, handleType)}};
}

int main()
{
	// Flush after every std::cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	initCommandMap();


	p_env = std::getenv("PATH");
	std::string env_p = p_env ? p_env : "";

	while (true)
	{
		std::cout << "$ ";

		std::string input;
		std::getline(std::cin, input);
		std::stringstream ss(input);
		std::string word;
		ss >> word;

		auto it = commandMap.find(word);
		if (it != commandMap.end())
		{
			it->second.cb(ss);
		}
		else
		{
			std::string arg;
			std::vector<char *> args;
			args.push_back(word.data());
			while (ss){
				ss >> arg;
				args.push_back(arg.data());
			}
			std::vector<std::string> split_path = split(env_p, path_delimiter);
			bool found = false;
			for (std::string str : split_path)
			{
				const fs::path pth(str);
				if (found) break;
				if (!fs::exists(pth)) continue;

				for (auto const &dir_entry : fs::recursive_directory_iterator(pth, fs::directory_options::skip_permission_denied))
				{
					if (word == dir_entry.path().filename().string())
					{
						if (access(dir_entry.path().c_str(), X_OK) != 0) continue;	// is not executable
						found = true;
						pid_t pid = fork();
						if (pid == -1)
							printf("fork failed\n");
						else if (pid == 0)
						{
							execvp(word.data(), args.data());
						}
						else
						{
							wait(NULL);
						}
						break;
					}
				}
			}

			if (!found)
				std::cout << word << ": command not found\n";
		}
	}
}
