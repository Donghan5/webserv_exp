#include "../includes/CgiHandler.hpp"
#include "../includes/AConfigBase.hpp"
#include "Logger.hpp"

CgiHandler::CgiHandler(const STR &scriptPath, const MAP<STR, STR> &env, const STR &body):
    _scriptPath(scriptPath), _env(env), _body(body), _cgi_pid(-1), _process_running(false),
    _start_time(0), _timeout(30)  // Add timeout initialization (30 seconds)
{
    _interpreters[".py"] = "/usr/bin/python3";
    _interpreters[".php"] = "/usr/bin/php";
    _interpreters[".pl"] = "/usr/bin/perl";
    _interpreters[".sh"] = "/bin/bash";

    // Initialize pipes with invalid values
    _input_pipe[0] = _input_pipe[1] = -1;
    _output_pipe[0] = _output_pipe[1] = -1;
}

CgiHandler::~CgiHandler() {
    closeCgi();
}

/*
	Convert to envp using execve function
*/
char **CgiHandler::convertEnvToCharArray(void) {
	char **envp = new char*[_env.size() + 1];
	int i = 0;
	MAP<STR, STR>::const_iterator it = _env.begin();
	for (; it != _env.end(); it++) {
		STR envEntry = it->first + "=" + it->second;
		envp[i] = strdup(envEntry.c_str());
		i++;
	}
	envp[i] = NULL;
	return envp;
}

/*
	args to launch cgi
*/
char **CgiHandler::convertArgsToCharArray(const STR &interpreter) {
	char **args = new char*[3];
	args[0] = strdup(interpreter.c_str());
	args[1] = strdup(_scriptPath.c_str());
	args[2] = NULL;
	return (args);
}

STR CgiHandler::createErrorResponse(const STR& status, const STR& message) {
    return "HTTP/1.1 " + status + " Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + Utils::intToString(message.length()) + "\r\n"
           "\r\n"
           + message;
}

bool CgiHandler::startCgi() {
    // Initialize pipes with error checking
    if (pipe(_input_pipe) == -1) {
        Logger::cerrlog(Logger::ERROR, "Failed to create input pipe: " + STR(strerror(errno)));
        _input_pipe[0] = _input_pipe[1] = -1;
        return false;
    }

    if (pipe(_output_pipe) == -1) {
        Logger::cerrlog(Logger::ERROR, "Failed to create output pipe: " + STR(strerror(errno)));
        // Clean up the previously created pipe
        close(_input_pipe[0]);
        close(_input_pipe[1]);
        _input_pipe[0] = _input_pipe[1] = -1;
        _output_pipe[0] = _output_pipe[1] = -1;
        return false;
    }

    // Set the output pipe to non-blocking mode for epoll
    // int flags = fcntl(_output_pipe[0], F_GETFL, 0);
    // if (flags == -1) {
    //     Logger::cerrlog(Logger::ERROR, "Failed to get flags for output pipe: " + STR(strerror(errno)));
    //     closeCgi(); // Use our improved closeCgi to clean up
    //     return false;
    // }

    if (fcntl(_output_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        Logger::cerrlog(Logger::ERROR, "Failed to set non-blocking mode: " + STR(strerror(errno)));
        closeCgi();
        return false;
    }

    // Check if the script exists and is executable
    if (access(_scriptPath.c_str(), F_OK | X_OK) != 0) {
        Logger::cerrlog(Logger::ERROR, "CGI script not executable: " + _scriptPath + " - " + STR(strerror(errno)));
        closeCgi();
        return false;
    }

    // Fork a child process
    _cgi_pid = fork();

    if (_cgi_pid < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to fork: " + STR(strerror(errno)));
        closeCgi();
        return false;
    }

    if (_cgi_pid == 0) {
        // Child process
        _start_time = time(NULL);
        // _timeout = 30; // 30 seconds timeout
        // Set up stdin from input pipe
        if (dup2(_input_pipe[0], STDIN_FILENO) == -1) {
            Logger::cerrlog(Logger::ERROR, "Child: Failed to redirect stdin: " + STR(strerror(errno)));
            exit(1);
        }
        close(_input_pipe[0]);
        close(_input_pipe[1]);

        // Set up stdout to output pipe
        if (dup2(_output_pipe[1], STDOUT_FILENO) == -1) {
            Logger::cerrlog(Logger::ERROR, "Child: Failed to redirect stdout: " + STR(strerror(errno)));
            exit(1);
        }
        close(_output_pipe[0]);
        close(_output_pipe[1]);

        // Convert environment variables for execve
        char **envp = convertEnvToCharArray();
        if (!envp) {
            Logger::cerrlog(Logger::ERROR, "Child: Failed to create environment");
            exit(1);
        }

        // Find the appropriate interpreter
        STR extension = _scriptPath.substr(_scriptPath.find_last_of("."));
        MAP<STR, STR>::const_iterator it = _interpreters.find(extension);
        if (it == _interpreters.end()) {
            Logger::cerrlog(Logger::ERROR, "Child: No interpreter for " + extension);
            for (size_t i = 0; envp[i] != NULL; i++) {
                free(envp[i]);
            }
            delete[] envp;
            exit(1);
        }

        // Prepare arguments for execve
        char **args = convertArgsToCharArray(it->second);
        if (!args) {
            Logger::cerrlog(Logger::ERROR, "Child: Failed to create args");
            for (size_t i = 0; envp[i] != NULL; i++) {
                free(envp[i]);
            }
            delete[] envp;
            exit(1);
        }

        Logger::cerrlog(Logger::INFO, "Executing: " + it->second + " " + _scriptPath);

        // Execute the script
        execve(args[0], args, envp);

        // If execve returns, an error occurred
        Logger::cerrlog(Logger::ERROR, "Child: execve failed: " + STR(strerror(errno)));

        // Clean up resources
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
        // Parent process if I set it, broken
        _start_time = time(NULL);

        // Close unused pipe ends
        close(_input_pipe[0]);
        close(_output_pipe[1]);

        _process_running = true;

        // Write request body to CGI script's stdin
        if (!_body.empty()) {
            Logger::cerrlog(Logger::DEBUG, "Sending body to CGI (size: " + Utils::intToString(_body.size()) + " bytes)");
            if (!writeToCgi(_body.c_str(), _body.size())) {
                Logger::cerrlog(Logger::ERROR, "Failed to write request body to CGI");
                closeCgi();
                return false;
            }
        }

        // Always close input pipe after writing
        if (_input_pipe[1] >= 0) {
            close(_input_pipe[1]);
            _input_pipe[1] = -1;
        }

        return true;
    }
}

bool CgiHandler::writeToCgi(const char* data, size_t len) {
    if (!_process_running || _input_pipe[1] < 0) {
        Logger::cerrlog(Logger::ERROR, "Cannot write to CGI: process not running or pipe closed");
        return false;
    }

    //map to bytes written by fds
    // static MAP<pid_t, ssize_t> total_written;
    ssize_t bytes_written = 0;
    size_t total_written = 0;

    // Write all data in chunks
    while (total_written < len) {

        bytes_written = write(_input_pipe[1], data + total_written, len - total_written);

        if (bytes_written < 0) {
            // Handle temporary errors (would block)
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Try again after a tiny sleep
                // struct timespec ts = { 0, 1000000 }; // 1ms
                // nanosleep(&ts, NULL);
                continue;
            }

            // Real error
            Logger::cerrlog(Logger::ERROR, "Write to CGI failed: " + STR(strerror(errno)));
            return false;
        }

        // Update the amount written
        total_written += bytes_written;
    }


    Logger::cerrlog(Logger::DEBUG, "Successfully wrote " + Utils::intToString(total_written) +
                    " bytes to CGI input");
    return true;
}

STR CgiHandler::readFromCgi() {
    if (!_process_running || _output_pipe[0] < 0) {
        return "";
    }

    char buffer[8192];
    STR output;
    ssize_t bytesRead;

    // Read available data from the CGI process
    while ((bytesRead = read(_output_pipe[0], buffer, sizeof(buffer))) > 0) {
    // if ((bytesRead = read(_output_pipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytesRead);
    }
    if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {

    // if (bytesRead < 0) {
        Logger::cerrlog(Logger::ERROR, "Failed to read from CGI: " + STR(strerror(errno)));
    }

    _output_buffer += output;
    return output;
}

bool CgiHandler::checkCgiStatus() {
    if (!_process_running) {
        return true; // Already done
    }

	// closeCgi();
	// return true; // Report as completed (timed out)

    // Check for timeout
    if ((time(NULL) - _start_time) > _timeout) {
		// log start tiem
		Logger::cerrlog(Logger::DEBUG, "CgiHandler::checkCgiStatus: CGI process started at " +
					   Utils::intToString(_start_time) + ", current time: " + Utils::intToString(time(NULL)));
        Logger::cerrlog(Logger::WARNING, "CGI process timed out after " +
                       Utils::intToString(_timeout) + " seconds");
        closeCgi();
        return true; // Report as completed (timed out)
    }

    if (_cgi_pid <= 0) {
        _process_running = false;
        return true;
    }

    // if (_process_running && (time(NULL) - _start_time > _timeout)) {
    //     Logger::cerrlog(Logger::WARNING, "CGI process timed out after " +
    //                   Utils::intToString(_timeout) + " seconds");
    //     closeCgi();
    //     return true; // Report as completed (timed out)
    // }

    int status;
    pid_t result = waitpid(_cgi_pid, &status, WNOHANG);

	//print info and if is parent
	Logger::cerrlog(Logger::DEBUG, "IS_PARENT: " + Utils::intToString(getppid()));
	Logger::cerrlog(Logger::DEBUG, "CgiHandler::checkCgiStatus: CGI process started at " +
				   Utils::intToString(_start_time) + ", current time: " + Utils::intToString(time(NULL)));
	Logger::cerrlog(Logger::DEBUG, "CgiHandler::checkCgiStatus: CGI process PID: " +
				   Utils::intToString(_cgi_pid) + ", status: " + Utils::intToString(status));

		//notify that result is not 0
		if (result != 0) {
			Logger::cerrlog(Logger::ERROR, "NOT ZERO: " +
						   Utils::intToString(_cgi_pid) + ", status: " + Utils::intToString(status));
		}

    if (result == 0) {
		std::cout << "Always here\n";
        // Process is still running
        return false;
    } else if (result == _cgi_pid) {
        // Process has exited
        _process_running = false;

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                Logger::cerrlog(Logger::INFO, "CGI process exited successfully");
            } else {
                Logger::cerrlog(Logger::WARNING, "CGI process exited with code: " +
                          Utils::intToString(exit_code));
            }
        } else if (WIFSIGNALED(status)) {
            Logger::cerrlog(Logger::WARNING, "CGI process terminated by signal: " +
                      Utils::intToString(WTERMSIG(status)));
        }
		closeCgi();
        return true;
    }
	else {
        // Error checking status
        Logger::cerrlog(Logger::ERROR, "Failed to check CGI status: " + STR(strerror(errno)));
        _process_running = false;
		closeCgi();
        return true;
    }
}

void CgiHandler::closeCgi() {
    // Log when we're cleaning up resources
    Logger::cerrlog(Logger::DEBUG, "CgiHandler::closeCgi: Cleaning up resources");

    // Close pipes carefully with proper error checking
    if (_input_pipe[0] >= 0) {
        if (close(_input_pipe[0]) < 0) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close input pipe[0]: " + STR(strerror(errno)));
        }
        _input_pipe[0] = -1;
    }

    if (_input_pipe[1] >= 0) {
        if (close(_input_pipe[1]) < 0) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close input pipe[1]: " + STR(strerror(errno)));
        }
        _input_pipe[1] = -1;
    }

    if (_output_pipe[0] >= 0) {
        if (close(_output_pipe[0]) < 0) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close output pipe[0]: " + STR(strerror(errno)));
        }
        _output_pipe[0] = -1;
    }

    if (_output_pipe[1] >= 0) {
        if (close(_output_pipe[1]) < 0) {
            Logger::cerrlog(Logger::DEBUG, "Failed to close output pipe[1]: " + STR(strerror(errno)));
        }
        _output_pipe[1] = -1;
    }

    // If the process is still running, terminate it with grace period
    if (_process_running && _cgi_pid > 0) {
        Logger::cerrlog(Logger::INFO, "Terminating CGI process " + Utils::intToString(_cgi_pid));

        // First try SIGTERM for graceful shutdown
        kill(_cgi_pid, SIGTERM);

        // Wait with timeout for process to exit
        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 100000000; // 100ms timeout

        int status;
        int ret = waitpid(_cgi_pid, &status, WNOHANG);

        if (ret == 0) {
            // Process didn't exit, wait a bit
            // nanosleep(&timeout, NULL);

            // Check again
            ret = waitpid(_cgi_pid, &status, WNOHANG);
            if (ret == 0) {
                // Still didn't exit, use SIGKILL as last resort
                Logger::cerrlog(Logger::WARNING, "CGI process didn't terminate gracefully, using SIGKILL");
                kill(_cgi_pid, SIGKILL);
                waitpid(_cgi_pid, &status, 0);
            }
        }

        _process_running = false;
        _cgi_pid = -1;
    }

    Logger::cerrlog(Logger::DEBUG, "CgiHandler::closeCgi: Resources cleaned up");
}
