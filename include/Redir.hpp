#include <string>

enum Direction{OUT, ERR};

class Redir{

    public:
        Direction direction;
        std::string directory;

        Redir(std::string dirn, std::string diry){
            directory = diry;
            if (dirn == ">" || dirn == "1>")
                direction = Direction::OUT;
            else if (dirn == "2>")
                direction = Direction::ERR;
        }
};