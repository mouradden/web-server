#include "Server.hpp"
#include <iostream>
#include <vector>
#include <list>
#include "httpstuff/Request.hpp"
#include "httpstuff/Response.hpp"
#include "parse/DataConfig.hpp"
#include <poll.h>
#include <algorithm>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

#define BUFFER_SIZE 4096
DataConfig  findConfig(ParseConfigFile configFile, std::string host, std::string port)
{
   
    std::vector<DataConfig> config = configFile.getData();
    for (size_t i = 0; i < config.size(); i++)
    {
        int count = 0;
        DataConfig currentConfig = config[i];
        if (currentConfig.getHost().compare(host) == 0)
            count += 1;
        std::vector<std::string> ports = currentConfig.getListen();
        for (size_t j = 0; j < ports.size(); j++)
        {
            if (ports[j].compare(port) == 0)
            {
                count += 1;
                break;
            }
        }
        if (count == 2)
            return (currentConfig);
    }
    return (DataConfig());
}

std::string prepareResponse(const std::string& body)
{
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.length() << "\r\n\r\n"
             << body;
    return response.str();
}

std::string    executeCGI(int clientSocket, Request& req, char **envp)
{
    std::vector<std::pair<std::string, std::string> > env;
    
    env.push_back(std::make_pair(std::string("REQUEST_METHOD"), req.getRequestMethod()));
    env.push_back(std::make_pair(std::string("CONTENT_TYPE"), req.getHeader("Content-Type")));
    env.push_back(std::make_pair(std::string("CONTENT_LENGTH"), req.getHeader("Content-Length")));
    env.push_back(std::make_pair(std::string("SCRIPT_NAME"), req.getRequestRessource()));

    int i = 0;
    while (envp[i])
    {
        std::string str(envp[i]);
        std::string::size_type pos = str.find('=');
        if (pos != std::string::npos) {
            std::string key = str.substr(0, pos);
            std::string value = str.substr(pos + 1);
            env.push_back(std::make_pair(key, value));
        }
        i++;
    }

    char **environement = new char*[env.size() + 1];
    for (size_t i = 0; i < env.size(); i++) {
        std::string str = env[i].first + "=" + env[i].second;
        environement[i] = new char[str.size() + 1];
        std::strcpy(environement[i], str.c_str());
    }
    environement[env.size()] = NULL;
    
    int pipes[2];
    std::string empty = "";
    pipe(pipes);
(void)clientSocket;

    int pid = fork();
    std::string newBody;
    if (pid == -1)
        {std::cerr << "here1\n";return empty;}
    else if (pid == 0)
    {
        char *args[] = {(char*) "/usr/bin/php", (char*)"upload.php", NULL};
 
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        execve(args[0], args, environement);
        std::cerr << "execve failed\n";
        exit(1); // If execve returns, it must have failed. Exit the child process.
    }
    else
    {
        char buffer[1024] = {0};
        int status;
        close(pipes[1]); // Close the write end of the pipe in the parent process.
        while (read(pipes[0], buffer, sizeof(buffer)-1) > 0)
        {
            newBody += buffer;
            memset(buffer, 0, sizeof(buffer));
        }
        waitpid(pid, &status, 0); // Wait for the child process to finish.
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {std::cerr << "here2\n";return empty;}
    std::cerr << "new Body = |" << newBody << "|\n";
    }

	close(pipes[1]);
	close(pipes[0]);

	// for (size_t i = 0; env[i]; i++)
	// 	delete[] env[i];
	// delete[] env;
	return (prepareResponse(newBody));

}

int main(int ac, char **av, char **envp)
{
    (void)ac;(void)av;
    Server server;
    ParseConfigFile config;
    config.parser("./parse/configfile.txt");
    for (size_t i = 0; i < config.getData().size(); i++)
    {
        server.createSocket(config.getData()[i]);
    }
    
    server.createServer(config.getData());
    server.putServerOnListening();

    std::map<int, Client> Clients;
    std::vector<pollfd> fds;
    for (size_t i = 0; i < server.getServerSockets().size(); i++)
    {
        int fd = server.getServerSocket(i);
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        fds.push_back(pfd);
        Clients.insert(std::make_pair(fd, Client()));
    }
    while (true) 
    {
        int pollResult = poll(fds.data(), fds.size(), 0);
        if (pollResult == -1)
        {
            std::cerr << "Error in poll\n";
            return 1;
        }
        std::vector<pollfd> requestClients;
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN)
            {
                std::vector<int> sockets = server.getServerSockets();
                std::vector<int>::iterator it = std::find(sockets.begin(), sockets.end(), fds[i].fd);
                if (it != sockets.end() && fds[i].fd == *it)
                {
                    int clientSocket = accept(fds[i].fd, NULL, NULL);
                    if (clientSocket == -1)
                    {
                        std::cerr << "Error accepting client connection\n";
                        close(fds[i].fd);
                    }
                    else
                    {
                        if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1) {
                            std::cerr << "Error setting socket to non-blocking\n";
                            return 1;
                        }
                        
                        std::cout << "New client connected, socket: " << clientSocket << "\n";
                        Clients.insert(std::make_pair(clientSocket, Client()));
                        pollfd pfd;
                        pfd.fd = clientSocket;
                        pfd.events = POLLIN;
                        requestClients.push_back(pfd);
                        fds.push_back(pfd);
                        server.setServer(clientSocket, server.getServers()[fds[i].fd]);
                    }
                }
            }
        }
        for (size_t i = 0; i < fds.size(); i++)
        {
            std::vector<int> sockets = server.getServerSockets();
            std::vector<int>::iterator it = std::find(sockets.begin(), sockets.end(), fds[i].fd);
            if (fds[i].revents & POLLIN && it == sockets.end())
            {
                int clientSocket = fds[i].fd;
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
                if (bytesRead > 0)
                {
                    DataConfig configData = server.getServers()[clientSocket];
                    // std::cout << "\n************OLD**************\n";
                    // configData.printDataConfig();
                    // std::cout << "\n***************NEW***********\n";
                    // c.printDataConfig();
                    // exit(1);
                    Clients[clientSocket].getRequestBuffer().append(buffer, bytesRead);
                    if (Clients[clientSocket].getRequestBuffer().find("Transfer-Encoding: chunked") != std::string::npos)
                    {
                        // std::cout << "********************chunked*******\n";
                        // Clients[clientSocket].getRequestBuffer().append(buffer, bytesRead);
                        // std::cout << "--------------> request before : |" << GREEN << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                        if (Clients[clientSocket].getRequestBuffer().find("\r\n0") != std::string::npos)
                        {
                            server.parseChunkedRequest(Clients[clientSocket].getRequestBuffer());
                        // std::cout << "--------------> request after : |" << RED << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                            Request req(Clients[clientSocket].getRequestBuffer());
                            Response response = req.handleRequest(configData);
                            Clients[clientSocket].setResponse(response.getResponseEntity());
                            fds[i].events |= POLLOUT;
                            fds[i].events &= ~POLLIN;
                        }
                    }
                    else
                    {
                        if (Clients[clientSocket].getRequestBuffer().find("\r\n\r\n") != std::string::npos)
                        {
                            // std::cout << "********************request *******\n";
                            std::cout << "--------------> request before : \n|" << GREEN << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                            Request req(Clients[clientSocket].getRequestBuffer());
                            std::cout << RED << "Host :" << req.getHeader("Host") << RESET << "\n";
                    // DataConfig c = findConfig(config, req.getHeader(), std::string("8081"));
                            std::string resource = req.getRequestRessource();
                            if (resource.substr(resource.find_last_of(".") + 1) == "php")
                            {
                                Clients[clientSocket].setResponse(executeCGI(clientSocket, req, envp));
                                fds[i].events |= POLLOUT;
                                fds[i].events &= ~POLLIN;
                            }
                            else
                            {
                                Response response = req.handleRequest(configData);
                                Clients[clientSocket].setResponse(response.getResponseEntity());
                                fds[i].events |= POLLOUT;
                                fds[i].events &= ~POLLIN;

                            }
                        }
                    }
                } 
                else if (bytesRead == 0)
                {
                    std::cout << "Client disconnected, socket: " << clientSocket << "\n";
                    std::cout << "fds size = " << fds.size() << "\n";
                    std::cout << "Clients size = " << Clients.size() << "\n";
                    close(clientSocket);
                    fds.erase(fds.begin() + i);
                    Clients.erase(clientSocket);
                }
                else
                {
                    // std::cerr << "recv return  = " << bytesRead << "\n";
                    // std::cerr << "Error receiving data from client, socket: " << clientSocket << "\n";
                    // perror("recv");
                    close(clientSocket);
                    fds.erase(fds.begin() + i);
                    Clients.erase(clientSocket);
                }
            }
        }
        for (size_t i = 0; i < fds.size(); i++)
        {
            // std::cout << "write socket = " << fds[i].fd << "\n";
            if (fds[i].revents & POLLOUT)
            {
                int clientSocket = fds[i].fd;
                std::cout << "Handling response for socket: " << clientSocket << std::endl;
                if (server.sendResponse(clientSocket, Clients[clientSocket]) == 0)
                {
                    std::cout << "Response sent for socket: " << clientSocket << "\n";
                    fds[i].events &= ~POLLOUT;
                    fds[i].events |= POLLIN;
                    Clients[clientSocket].getRequestBuffer().clear();
                    Clients[clientSocket].getResponseBuffer().clear();
                    Clients[clientSocket].setOffset(0);
                }
            }
        }
    }
    return 0;
}
