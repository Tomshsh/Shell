#include <string>
#include <fcntl.h>

enum Direction
{
    OUT,
    ERR
};
enum Action
{
    APPEND,
    TRUNCATE
};

class Redir
{

public:
    Direction direction;
    uint32_t action;
    std::string directory;

    Redir(std::string dirn, std::string diry) : directory(std::move(diry))
    {
        action = (dirn.find(">>")!= std::string::npos)? O_APPEND : O_TRUNC;
        if (!dirn.empty() && dirn.at(0) == '2')
            direction = Direction::ERR;
        else
            direction = Direction::OUT;
    }

    void redirectInput(int out_fd, int err_fd)
    {
        ssize_t bytes_read;
        char buffer[4096];
        int file = open(directory.c_str(), O_CREAT | O_WRONLY | action | O_CLOEXEC, S_IRUSR | S_IWUSR);
        int select_fd = direction == OUT ? out_fd : err_fd;
        while ((bytes_read = read(select_fd, buffer, sizeof(buffer))) > 0)
            if (write(file, buffer, bytes_read) != bytes_read)
                std::cerr << "Error writing to file\n";
        close(select_fd);
        close(file);
    }
};