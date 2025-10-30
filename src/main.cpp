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
	std::function<void(std::vector<std::string> &)> cb;

	Command(std::string cmd, command_type cmd_type, std::function<void(std::vector<std::string> &)> callback) : command(cmd), type(cmd_type), cb(std::move(callback)) {};
};

std::unordered_map<std::string, Command> commandMap;

const char *path_delimiter = []
{
#ifdef _WIN32
	return ";";
#else
	return ":";
#endif
}();

char *p_env = {};

/////////////////////////////# UTILS #///////////////////////////////

std::string typeToStr(command_type cmd)
{
	switch (cmd)
	{
	case BUILTIN:
		return "builtin";

	default:
		return "";
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

std::string findDirInPath(std::string word)
{
	std::string env_p(p_env);
	std::vector<std::string> split_path = split(env_p, path_delimiter);
	for (std::string str : split_path)
	{
		const fs::path pth(str);
		if (!fs::exists(pth))
			continue;

		for (auto const &dir_entry : fs::recursive_directory_iterator(pth, fs::directory_options::skip_permission_denied))
		{
			if (word == dir_entry.path().filename().string())
			{
				if (access(dir_entry.path().c_str(), X_OK) != 0)
					continue; // is not executable
				return dir_entry.path().c_str();
			}
		}
	}

	return {};
}

std::vector<std::string> parseArgs(std::string &line)
{
	std::string arg;
	std::vector<std::string> args;
	arg.clear();
	bool inDoubleQuotes = false;
	for (auto it = line.begin(); it != line.end(); ++it)
	{
		if (!inDoubleQuotes)
		{
			if (*it == ' ' || *it == '\0')
			{
				if (!arg.empty())
				{
					args.push_back(arg);
					arg.clear();
				}
				continue;
			}
			else if (*it == '\'')
			{
				size_t startPos = it - line.begin() + 1; // skip the character in check
				size_t endPos = line.find('\'', startPos);
				if (endPos == std::string::npos)
				{
					arg += *it;
					continue;
				}
				arg += line.substr(startPos, endPos - startPos);
				it = line.begin() + endPos; // now *it == '\'', but next iteration will set it to one character after
				continue;
			}
		}
		if (*it == '\"')
			inDoubleQuotes = !inDoubleQuotes;
		else if (*it == '\\')
			arg += *(++it);
		else
			arg += *it;
	}
	return args;

}

/////////////////////////////# METHODS #///////////////////////////////

void handleExit(std::vector<std::string> &vec)
{
	if (vec.size() > 2)
	{
		printf("exit: too many arguments\n");
		return;
	}
	if (vec.size() < 2)
		exit(0);

	std::string code = vec.at(1);
	int exit_code = 0;
	if (code.length() > 0)
	{
		if (code.length() > 1 || !(code.at(0) == '0' || code.at(0) == '1'))
		{
			printf("exit: invalid argument\n");
			return;
		}
		exit_code = code.at(0) - '0';
	}
	exit(exit_code);
}

void handleEcho(std::vector<std::string> &vec)
{
	for (auto it = vec.begin() + 1; it != vec.end(); ++it)
		std::cout << *it << ' ';
	std::cout << "\n";
}

void handleType(std::vector<std::string> &vec)
{
	for (auto com = vec.begin() + 1; com != vec.end(); ++com)
	{
		std::string env_p = p_env ? p_env : "";

		auto it = commandMap.find(*com);
		if (it != commandMap.end())
		{
			printf("%s is a shell %s\n", (*com).c_str(), typeToStr(it->second.type).c_str());
			continue;
		}
		else
		{
			std::string dir_entry = findDirInPath(*com);
			if (dir_entry.length())
			{
				std::cout << *com << " is " << dir_entry << "\n";
				continue;
			}
		}
		printf("%s: not found\n", (*com).c_str());
	}
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
		input += '\0';

		std::vector<std::string> args = parseArgs(input);

		auto it = commandMap.find(args[0]);
		if (it != commandMap.end())
		{
			it->second.cb(args);
		}
		else
		{
			std::vector<char *> c_args;
			for (const auto &a : args)
				c_args.push_back(const_cast<char *>(a.c_str()));

			std::string dir_entry = findDirInPath(args[0]);
			if (dir_entry.empty())
			{
				std::cout << args[0] << ": command not found\n";
				continue;
			}

			pid_t pid = fork();
			if (pid == -1)
				printf("fork failed\n");
			else if (pid == 0)
			{
				execvp(dir_entry.c_str(), c_args.data());
				perror("execvp failed");
				exit(1);
			}
			else
			{
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}
}
