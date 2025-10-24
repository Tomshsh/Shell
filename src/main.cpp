#include <iostream>
#include <string>

int main()
{
	// Flush after every std::cout / std:cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	while (true)
	{
		std::cout << "$ ";

		std::string input;
		std::getline(std::cin, input);
		
		if (input.compare(0, 4, "exit") == 0)
		{
			int exit_code = '0';
			if (input.length() > 5)
			{
				exit_code = input.at(5);
			}
			if (exit_code == '0' || exit_code == '1')
			{
				exit_code = exit_code - '0';
			exit(exit_code);
			}
			else
				printf("exit: bad argument\n");
		}

		else if (input.compare(0, 4, "echo") == 0)
		{
			printf("%s\n", input.data() + 5);
		}

		else if (input.compare(0, 4, "type") == 0)
		{
			if (input.compare(5, 4, "echo") == 0 || input.compare(5, 4, "exit") == 0 || input.compare(5, 4, "type") == 0)
				printf("%s is a shell builtin\n", input.c_str() + 5);
			else
				printf("%s: not found\n", input.c_str() + 5);
		}

		else
		{
			std::cout << input << ": command not found" << std::endl;
		}
	}
}
