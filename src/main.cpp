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
			if (input.length() > 5) {
				exit_code = input.at(5);
			}

			if (exit_code == '0')
				std::cout << "Exit with status 0 (success)" << std::endl;
			if (exit_code == '1')
				std::cout << "Exit with status 1 (error)" << std::endl;
			
			exit(exit_code);
		}

		std::cout << input << ": command not found" << std::endl;
	}
}
