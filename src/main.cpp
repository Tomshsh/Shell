#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>

enum command_type
{
	BUILTIN
};

struct Command
{
	std::string command;
	command_type type;
	std::function<void(std::stringstream&)> cb;

	Command(std::string cmd, command_type cmd_type, std::function<void(std::stringstream&)> callback) : command(cmd), type(cmd_type), cb(std::move(callback)) {};
};

std::unordered_map<std::string, Command> commandMap;

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

void handleExit(std::stringstream& ss)
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

void handleEcho(std::stringstream& ss)
{
	ss.seekg(5);
	std::cout << ss.rdbuf() << "\n";
}

void handleType(std::stringstream& ss)
{
	std::string com;
	ss >> com;
	auto it = commandMap.find(com);
	if (it == commandMap.end()){
		printf("%s: not found\n", com.c_str());
		return;
	}
	printf("%s is a shell %s\n", com.c_str(), TypeToStr(it->second.type).c_str());
}

void initCommandMap() {
	commandMap = {
		{"exit", Command("exit", BUILTIN, handleExit)},
		{"echo", Command("echo", BUILTIN, handleEcho)},
		{"type", Command("type", BUILTIN, handleType)}
	};
}

int main()
{
	// Flush after every std::cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	initCommandMap();

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
			std::cout << input << ": command not found" << std::endl;
		}
	}
}
