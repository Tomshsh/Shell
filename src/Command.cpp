#include <Command.hpp>
#include <fstream>
#include <string.h>
#include <sys/wait.h>
#include <sstream>
#include <ostream>
#include <math.h>

//////////////////////////////////////// UTILS ////////////////////////////////////////////////////////////////

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

int extractIntVal(std::string &str)
{
	int val = 0;
	for (int i = 0; i < str.length(); i++)
	{
		char ch = str.at(i);
		if (!std::isdigit(static_cast<unsigned char>(ch)))
			return sizeof(int);
		val += pow(10, i) + (ch - '0') - 1;
	}
	return val;
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

void drainAndWrite(int fd, std::ostream &out)
{
	char buffer[4096];
	ssize_t bytes_read;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		out.write(buffer, bytes_read);
}

//////////////////////////////////////// BUILTIN HANDLERS ////////////////////////////////////////

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
	auto it = vec.begin() + 1;
	if (it == vec.end())
		return;

	std::cout << *it;
	++it;

	for (; it < vec.end(); ++it)
		std::cout << ' ' << *it;
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

void handleHistory(std::vector<std::string> &vec)
{
	HISTORY_STATE *state = history_get_history_state();
	std::streambuf *sb = std::cout.rdbuf();
	int length = state->length;

	if (vec.size() > 2)
	{
		if (vec.at(1) == "-r")
		{
			std::ifstream file(vec.at(2));
			std::string line;
			while (std::getline(file, line))
			{
				if (line.empty())
					continue;
				add_history(line.data());
			}
			return;
		}

		if (vec.at(1) == "-w")
		{
			std::ofstream fb;
			fb.open(vec.at(2));
			for (int i = 0; i < state->length; i++)
				fb << history_get(history_base + i)->line << '\n';
			return;
		}

		if (vec.at(1) == "-a")
		{
			static int i = 0;
			std::ofstream fb;
			fb.open(vec.at(2), std::ios::app);
			for (; i < state->length; i++)
				fb << history_get(history_base + i)->line << '\n';
			return;
		}
	}
	else if (vec.size() > 1)
	{
		int val = extractIntVal(vec.at(1));
		length = val < length ? val : length;
	}

	int i = state->length - length;
	for (; i < state->length; i++)
		printf("%4d  %s\n", i + 1, history_get(history_base + i)->line);
}

void handlePwd(std::vector<std::string> &vec)
{
	auto pth = fs::current_path();
	std::cout << pth.c_str() << '\n';
}

void handleCd(std::vector<std::string> &vec)
{
	std::string path = vec[1];
	if (path == "~")
		path = getenv("HOME");
	fs::path pth(path);
	if (fs::exists(pth))
		fs::current_path(path);
}

std::unordered_map<std::string, std::function<void(std::vector<std::string> &)>> builtins = {
	{"exit", handleExit},
	{"echo", handleEcho},
	{"type", handleType},
	{"history", handleHistory},
	{"pwd", handlePwd},
	{"cd", handleCd}};

///////////////////////////////////////////////////// CLASS MEMBERS /////////////////////////////////////////////////////

void Command::attachRedir(std::string &direction, std::string &directory)
{
	_redirs.push_back(std::make_unique<Redir>(direction, directory));
}

void Command::run(int out_read)
{
	pid_t pid = 1;
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

	auto it = builtins.find(_argv[0]);
	if (it != builtins.end())
	{
		int saved_out = dup(STDOUT_FILENO);
		int saved_err = dup(STDERR_FILENO);

		dup2(out_pipe[1], STDOUT_FILENO);
		dup2(err_pipe[1], STDERR_FILENO);

		std::stringstream ss;
		drainAndWrite(out_read, ss);
		close(out_read);

		it->second(_argv);

		fflush(stdout);
		fflush(stderr);
		dup2(saved_out, STDOUT_FILENO);
		dup2(saved_err, STDERR_FILENO);
		close(saved_out);
		close(saved_err);
	}
	else
	{
		pid = fork();
		if (pid == -1)
			printf("fork failed\n");
		else if (pid == 0)
		{
			close(out_pipe[0]);
			close(err_pipe[0]);
			dup2(out_pipe[1], STDOUT_FILENO);
			dup2(err_pipe[1], STDERR_FILENO);
			dup2(out_read, STDIN_FILENO);
			close(out_pipe[1]);
			close(err_pipe[1]);
			close(out_read);

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

	if (pid > 0)
	{
		close(out_pipe[1]);
		close(err_pipe[1]);

		for (auto &redir : _redirs)
			redir->redirectInput(out_pipe[0], err_pipe[0]);

		if (_next != nullptr)
			_next->run(out_pipe[0]);

		drainAndWrite(out_pipe[0], std::cout);
		drainAndWrite(err_pipe[0], std::cerr);

		close(out_pipe[0]);
		close(err_pipe[0]);

		int status;
		waitpid(pid, &status, 0);
	}
}
