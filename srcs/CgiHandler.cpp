#include "../includes/CgiHandler.hpp"

/*
 * Convert to envp using execve function
*/
char **CgiHandler::convertEnvToCharArray(const std::map<std::string, std::string> &env) {
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
 * Convet to args using execve function
*/
char **CgiHandler::convertArgsToCharArray(const std::string &interpreter, const std::string &scriptPath) {
	char **args = new char*[3];
	args[0] = strdup(interpreter.c_str());
	args[1] = strdup(scriptPath.c_str());
	args[2] = NULL;
	return (args);
}

std::string CgiHandler::createErrorResponse(const std::string& status, const std::string& message) {
    return "HTTP/1.1 " + status + " Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + Utils::intToString(message.length()) + "\r\n"
           "\r\n"
           + message;
}
