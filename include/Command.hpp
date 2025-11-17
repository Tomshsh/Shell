#include <iostream>
#include <unistd.h>
#include <functional>
#include <Redir.hpp>
#include <unordered_map>
#include <vector>
#include <filesystem>

// Command.hpp
#ifndef COMMAND_HPP
#define COMMAND_HPP

namespace fs = std::filesystem;

extern std::unordered_map<std::string, std::function<void(std::vector<std::string> &)>> builtins;

std::vector<std::string> split(std::string str, std::string delim);

fs::path findDirInPath(std::string word);

void handleExit(std::vector<std::string> &vec);

void handleEcho(std::vector<std::string> &vec);

void handleType(std::vector<std::string> &vec);

class Command
{

private:
    std::vector<std::string> _argv;
    int _pipefd[2];
    std::vector<std::unique_ptr<Redir>> _redirs;
    std::unique_ptr<Command> _next;

public:
    Command() = default;
    Command(std::vector<std::string> argv): _argv(std::move(argv)), _next(nullptr){};

    void attachRedir(std::string& direction, std::string& directory);
    void setOrAppendCommand(std::vector<std::string>& argv)
    {
        if (_argv.empty())
        {
            _argv = argv;
            return;
        }

        Command *current = this;
        while(current->_next != nullptr)
            current = current->_next.get();

        current->_next = std::make_unique<Command>(argv);
    }

    void run();
};

#endif