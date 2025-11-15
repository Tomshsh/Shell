#include <iostream>
#include <string>
#include <span>
#include <Command.hpp>

class Parser{

    private:
        std::string _line;
        std::string::iterator _it;
        std::unique_ptr<Command> _command = nullptr;

        
        std::string nextArg();
        void nextLine();
        
    public:
        Parser(std::string line): _line(std::move(line)), _it(_line.begin()){}
        void runCommand();
        static char **character_name_completion(const char*, int, int);
        static char *command_name_generator(const char *, int state);
};