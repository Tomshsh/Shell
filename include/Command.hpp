#include <iostream>
#include <unistd.h>
#include <span>
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

class Command {

    private:
        std::vector<std::string> _argv;
        int _pipefd[2];
        std::vector<Redir *> _redirs;

        
    public:
        Command();
        Command(std::vector<std::string> argv){
            _argv = argv;
            _redirs = {};
        }
        // Command(const char* arg, std::span<const char* const> argv, int pipefd[2]): _arg(arg), _argv(argv), _pipefd(pipefd), _redirs({}){}

        void attachRedir(std::string direction, std::string directory);

        void run();
};

#endif