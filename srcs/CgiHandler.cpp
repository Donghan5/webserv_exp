#include "../includes/CgiHandler.hpp"

CgiHandler::CgiHandler(const std::string &scriptPath, const std::map<std::string, std::string> &env, const std::string &body): _scriptPath(scriptPath), _env(env), _body(body) {
	_interpreters[".py"] = "/usr/bin/python3";
	_interpreters[".php"] = "/usr/bin/php";
	_interpreters[".pl"] = "/usr/bin/perl";
	_interpreters[".sh"] = "/bin/bash";
}


std::string intToString(int num) {
	std::ostringstream oss;
	oss << num;

	return oss.str();
}

CgiHandler::~CgiHandler() {}

/*
	Convert to envp using execve function
*/
char **CgiHandler::convertEnvToCharArray(void) {
	char **envp = new char*[_env.size() + 1];
	int i = 0;
	std::map<std::string, std::string>::const_iterator it = _env.begin();
	for (; it != _env.end(); it++) {
		std::string envEntry = it->first + "=" + it->second;
		envp[i] = strdup(envEntry.c_str());
		i++;
	}
	envp[i] = NULL;
	return envp;
}

/*
	args to launch cgi
*/
char **CgiHandler::convertArgsToCharArray(const std::string &interpreter) {
	char **args = new char*[3];
	args[0] = strdup(interpreter.c_str());
	args[1] = strdup(_scriptPath.c_str());
	args[2] = NULL;
	return (args);
}

std::string CgiHandler::executeCgi() {
	int pipefd_in[2], pipefd_out[2];

	if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
		return "500 Internal Server Error\r\n\r\nPipe error";
	}

	pid_t pid = fork();
	if (pid < 0) {
		return "500 Internal Server Error\r\n\r\nFork error";
	}

	if (pid == 0) {
		dup2(pipefd_out[1], STDOUT_FILENO);
		close(pipefd_out[0]);
		close(pipefd_out[1]);

		dup2(pipefd_in[0], STDIN_FILENO);
		close(pipefd_in[0]);
		close(pipefd_in[1]);

		char **envp = convertEnvToCharArray();
		std::string extension = _scriptPath.substr(_scriptPath.find_last_of("."));
		std::map<std::string, std::string>::const_iterator it = _interpreters.find(extension);
		if (it == _interpreters.end()) { // free the memory
			for(size_t i = 0; envp[i] != NULL; i++) {
				free(envp[i]);
			}
			delete[] envp;
			exit(1);
		}

		char **args = convertArgsToCharArray(it->second);
		execve(args[0], args, envp);

		for (size_t i = 0; envp[i] != NULL; i++) {
			free(envp[i]);
		}
		delete[] envp;

		for (size_t i = 0; args[i] != NULL; i++) {
			free(args[i]);
		}
		delete[] args;
		exit(1);
	} else {
		close(pipefd_out[1]);
		close(pipefd_in[0]);

		if (!_body.empty()) {
			write(pipefd_in[1], _body.c_str(), _body.size());
		}
		close(pipefd_in[1]);

		char buffer[4096];
		std::string output;
		int byteRead;

		while ((byteRead = read(pipefd_out[0], buffer, sizeof(buffer))) > 0) {
			output.append(buffer, byteRead);
		}
		close(pipefd_out[0]);

		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + output;
		} else {
			return "500 Internal Server Error\r\n\r\nCGI Execution Failed";
		}
	}
}

std::string CgiHandler::createErrorResponse(const std::string& status, const std::string& message) {
    return "HTTP/1.1 " + status + " Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + Utils::intToString(message.length()) + "\r\n"
           "\r\n"
           + message;
}
