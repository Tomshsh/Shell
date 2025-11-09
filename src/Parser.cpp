#include <Parser.hpp>
#include <unordered_set>
#include <vector>

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
        if (arg == ">" || arg == "1>" || arg == "2>")
        {
            auto next_arg = nextArg();
            if (!next_arg.empty())
            {
                _command->attachRedir(arg, next_arg);
                continue;
            }

            if (!args.empty() && _command == nullptr)
                _command = std::make_unique<Command>(args);
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