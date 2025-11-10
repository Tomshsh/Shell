#include <string>
#include <fcntl.h>

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

        void redirectInput(int out_fd, int err_fd)
        {
            ssize_t bytes_read;
            char buffer[4096];
            int file = open(directory.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            int select_fd = direction == OUT ? out_fd : err_fd;
            while ((bytes_read = read(select_fd, buffer, sizeof(buffer))) > 0)
                if (write(file, buffer, bytes_read) != bytes_read)
                    std::cerr << "Error writing to file\n";
            close(select_fd);
            close(file);
        }
};