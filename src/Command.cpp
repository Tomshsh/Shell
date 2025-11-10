#include <Command.hpp>
#include <fstream>
#include <string.h>
#include <sys/wait.h>

std::vector<std::string> split(std::string str, std::string delim)
{
	std::vector<std::string> arr;
	char *d_str = (char *)str.data();
	char *d_delim = (char *)delim.data();
	char *tok = strtok(d_str, d_delim);

	while (tok != NULL)
	{
		arr.push_back(std::string(tok));
		tok = strtok(NULL, d_delim);
	}

	return arr;
}

fs::path findDirInPath(std::string word)
{
	std::string env_p(getenv("PATH"));
	std::vector<std::string> split_path = split(env_p, ":");
	for (std::string str : split_path)
	{
		const fs::path pth(str);
		if (!fs::exists(pth))
			continue;

		for (auto const &dir_entry : fs::directory_iterator(pth, fs::directory_options::skip_permission_denied))
		{
			if (word == dir_entry.path().filename().string())
			{
				if (access(dir_entry.path().c_str(), X_OK) != 0)
					continue; // is not executable
				return dir_entry.path();
			}
		}
	}

	return {};
}

void handleExit(std::vector<std::string> &vec)
{
	if (vec.size() > 2)
	{
		printf("exit: too many arguments\n");
		return;
	}
	if (vec.size() < 2)
		exit(0);

	std::string code = vec.at(1);
	int exit_code = 0;
	if (code.length() > 0)
	{
		if (code.length() > 1 || !(code.at(0) == '0' || code.at(0) == '1'))
		{
			printf("exit: invalid argument\n");
			return;
		}
		exit_code = code.at(0) - '0';
	}
	exit(exit_code);
}

void handleEcho(std::vector<std::string> &vec)
{
	for (auto it = vec.begin() + 1; it < vec.end(); ++it)
		std::cout << *it << ' ';
	std::cout << "\n";
}

void handleType(std::vector<std::string> &vec)
{
	for (auto com = vec.begin() + 1; com < vec.end(); ++com)
	{
		auto it = builtins.find(*com);
		if (it != builtins.end())
		{
			printf("%s is a shell builtin\n", com->c_str());
			continue;
		}
		else
		{
			std::string dir_entry = findDirInPath(*com).c_str();
			if (dir_entry.length())
			{
				std::cout << *com << " is " << dir_entry << "\n";
				continue;
			}
		}
		std::cerr << *com << ": not found\n";
	}
}

std::unordered_map<std::string, std::function<void(std::vector<std::string> &)>> builtins = {
	{"exit", handleExit},
	{"echo", handleEcho},
	{"type", handleType}};

Command::Command() {}

void Command::attachRedir(std::string direction, std::string directory)
{
	_redirs.push_back(std::make_unique<Redir>(direction, directory));
}

void Command::run()
{
	if (_argv[0].empty())
		return;

	int out_pipe[2], err_pipe[2];
	if (pipe(out_pipe) == -1)
	{
		perror("pipe failed");
		return;
	}
	if (pipe(err_pipe) == -1)
		perror("error pipe failed");

	pid_t pid = fork();
	if (pid == -1)
		printf("fork failed\n");
	else if (pid == 0)
	{
		close(out_pipe[0]);
		close(err_pipe[0]);
		dup2(out_pipe[1], STDOUT_FILENO);
		dup2(err_pipe[1], STDERR_FILENO);
		close(out_pipe[1]);
		close(err_pipe[1]);

		auto it = builtins.find(_argv[0]);
		if (it != builtins.end())
		{
			it->second(_argv);
			exit(1);
		}
		else
		{
			std::vector<char *> c_args;
			for (const auto &a : _argv)
				c_args.push_back(const_cast<char *>(a.c_str()));
			c_args.push_back(nullptr);

			std::string dir_entry = findDirInPath(_argv[0]);
			if (dir_entry.empty())
			{
				std::cerr << _argv[0] << ": command not found\n";
				exit(1);
			}

			execv(dir_entry.c_str(), c_args.data());
			perror("execvp failed (dir underneath)");
			printf("\"%s\" \n", dir_entry.c_str());
			exit(1);
		}
	}
	else
	{
		close(out_pipe[1]);
		close(err_pipe[1]);

		for (auto& redir : _redirs)
			redir->redirectInput(out_pipe[0], err_pipe[0]);

		std::string output;
		char buffer[4096];
		ssize_t bytes_read;

		while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
			output.append(buffer, bytes_read);
		
		std::cout << output;
		output.clear();
		
		while ((bytes_read = read(err_pipe[0], buffer, sizeof(buffer))) > 0)
			output.append(buffer, bytes_read);
		
		std::cerr << output;

		close(out_pipe[0]);
		close(err_pipe[0]);


		int status;
		waitpid(pid, &status, 0);
	}
}
