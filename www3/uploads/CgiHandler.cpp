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

std::string CgiHandler::executeProxy() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return createErrorResponse("500", "Socket creation failed");
    }

    // Set up server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(_env["SERVER_PORT"].c_str()));
    
    // Convert hostname to IP
    struct hostent *server = gethostbyname(_env["HTTP_HOST"].c_str());
    if (server == NULL) {
        close(sockfd);
        return createErrorResponse("500", "Host resolution failed");
    }
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return createErrorResponse("500", "Connection failed");
    }

    // Update path handling to include query string
    std::string path = _env["SCRIPT_NAME"];
    if (!_env["QUERY_STRING"].empty()) {
        path += "?" + _env["QUERY_STRING"];
    }
    
    // Prepare HTTP request
    std::string request = _env["REQUEST_METHOD"] + " " + path + " " + _env["SERVER_PROTOCOL"] + "\r\n";
    request += "Host: " + _env["HTTP_HOST"] + "\r\n";
    request += "Content-Type: application/x-www-form-urlencoded\r\n";
    
    if (!_env["HTTP_COOKIE"].empty()) {
        request += "Cookie: " + _env["HTTP_COOKIE"] + "\r\n";
    }
    
    if (!_body.empty()) {
        request += "Content-Length: " + intToString(_body.length()) + "\r\n";
    }
    
    request += "Connection: close\r\n";  // Add Connection: close header
    request += "\r\n";
    if (!_body.empty()) {
        request += _body;
    }

    std::cerr << "DEBUG - Full proxy request:\n" << request << std::endl;

    // Send request
    if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
        close(sockfd);
        return createErrorResponse("500", "Send failed");
    }

    // Receive response
    std::string response;
    char buffer[4096];
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        response += buffer;
    }

    close(sockfd);
    return response;
}

std::string CgiHandler::createErrorResponse(const std::string& status, const std::string& message) {
    return "HTTP/1.1 " + status + " Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + intToString(message.length()) + "\r\n"
           "\r\n"
           + message;
}
