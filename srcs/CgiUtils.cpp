#include "CgiUtils.hpp"

/*
	Convert to envp using execve function
*/
char **CgiUtils::convertEnvToCharArray(std::map<std::string, std::string> &env) {
	char **envp = new char*[env.size() + 1];
	int i = 0;
	std::map<std::string, std::string>::const_iterator it = env.begin();
	for (; it != env.end(); it++) {
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
char **CgiUtils::convertArgsToCharArray(const std::string &interpreter, const std::string &scriptPath) {
	if (interpreter.empty() || scriptPath.empty()) {
		return NULL;
	}

	char **args = new char*[3];
	args[0] = strdup(interpreter.c_str());
	args[1] = strdup(scriptPath.c_str());
	args[2] = NULL;
	return (args);
}

// close all pipes safely
void CgiUtils::closePipes(int input_pipe0, int input_pipe1, int output_pipe0, int output_pipe1) {
    if (input_pipe0 >= 0) {
        if (close(input_pipe0) < 0 && errno != EBADF) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close input pipe[0]: " + std::string(strerror(errno)));
        }
    }

    if (input_pipe1 >= 0) {
        if (close(input_pipe1) < 0 && errno != EBADF) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close input pipe[1]: " + std::string(strerror(errno)));
        }
    }

    if (output_pipe0 >= 0) {
        if (close(output_pipe0) < 0 && errno != EBADF) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close output pipe[0]: " + std::string(strerror(errno)));
        }
    }

    if (output_pipe1 >= 0) {
        if (close(output_pipe1) < 0 && errno != EBADF) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close output pipe[1]: " + std::string(strerror(errno)));
        }
    }
}

std::string CgiUtils::createErrorResponseCgi(const std::string& status, const std::string& message) {
    return "HTTP/1.1 " + status + " Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + Utils::intToString(message.length()) + "\r\n"
           "\r\n"
           + message;
}
