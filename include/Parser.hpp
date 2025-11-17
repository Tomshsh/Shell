#include <iostream>
#include <string>
#include <span>
#include <Command.hpp>

class Parser{

    private:
        std::string _line;
        std::string::iterator _it;
        std::unique_ptr<Command> _command;

        
        std::string nextArg();
        void nextLine();
        
    public:
        Parser(std::string line): _line(std::move(line)), _it(_line.begin()), _command(std::make_unique<Command>()){}
        void runCommand();
        static char **character_name_completion(const char*, int, int);
        static char *command_name_generator(const char *, int state);
};