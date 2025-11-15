#include <Parser.hpp>
#include <unordered_set>
#include <vector>
#include <readline/readline.h>
#include <readline/history.h>

char **Parser::character_name_completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_name_generator);
}

char *Parser::command_name_generator(const char *text, int state)
{
    static std::vector<std::string> split_path;
    static std::vector<std::string>::iterator path_it;
    static fs::directory_iterator dir_it;
    static fs::directory_iterator dir_end;
    static std::unordered_map<std::string, std::function<void(std::vector<std::string> &)>>::iterator builtin_it;

    if (!state)
    {
        split_path = split(getenv("PATH"), ":");
        path_it = split_path.begin();
        dir_it = dir_end;
        builtin_it = builtins.begin();
    }

    for (; builtin_it != builtins.end(); builtin_it++)
        if (builtin_it->first.find(text) == 0)
            return strdup((builtin_it++)->first.c_str());

    for (; path_it != split_path.end(); path_it++)
    {
        fs::path pth(*path_it);
        if (!fs::exists(pth))
            continue;

        if (dir_it == dir_end)
            dir_it = fs::directory_iterator(pth, fs::directory_options::skip_permission_denied);

        for (; dir_it != dir_end; dir_it++)
        {
            if (access(dir_it->path().c_str(), X_OK) != 0)
                continue; // is not executable
            if (dir_it->path().filename().string().find(text) == 0)
            {
                char *str = strdup(dir_it->path().filename().c_str());
                dir_it++;
                return str;
            }
        }
    }
    return NULL;
}

std::string Parser::nextArg()
{
    std::string arg;
    arg.clear();
    bool inDoubleQuotes = false;
    std::unordered_set<char> escapableChars = {'$', '\"', '\\', '\n', '`'};
    for (; _it < _line.end(); _it++)
    {
        if (!inDoubleQuotes)
        {
            if (*_it == ' ' || *_it == '\0')
            {
                if (!arg.empty())
                    break;
                continue;
            }
            else if (*_it == '\'')
            {
                size_t startPos = _it - _line.begin() + 1; // skip the character in check
                size_t endPos = _line.find('\'', startPos);
                if (endPos == std::string::npos)
                {
                    arg += *_it;
                    continue;
                }
                arg += _line.substr(startPos, endPos - startPos);
                _it = _line.begin() + endPos; // now *it == '\'', but next iteration will set it to one character after
                continue;
            }
        }
        if (*_it == '\"')
            inDoubleQuotes = !inDoubleQuotes;
        else if (*_it == '\\')
        {
            if (inDoubleQuotes && escapableChars.count(*(_it + 1)) == 0)
                arg += *_it; // if in quotes andnext char not escapable, insert both
            arg += *(++_it);
        }
        else
            arg += *_it;
    }
    return arg;
}

void Parser::nextLine()
{
    std::vector<std::string> args;
    for (auto arg = nextArg(); !arg.empty(); arg = nextArg())
    {
        if (arg == ">" || arg == "1>" || arg == "2>" || arg == ">>" || arg == "1>>" || arg == "2>>")
        {
            if (!args.empty() && _command == nullptr)
                _command = std::make_unique<Command>(args);

            auto next_arg = nextArg();
            if (!next_arg.empty())
            {
                _command->attachRedir(arg, next_arg);
                continue;
            }
        }
        else if (arg == "|")
        {
        }
        args.push_back(arg);
    }
    if (args.empty())
        args.push_back("\0");

    if (_command == nullptr)
        _command = std::make_unique<Command>(args);
}

void Parser::runCommand()
{
    nextLine();
    if (_command != nullptr)
        _command->run();
}