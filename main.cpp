#include "Server.hpp"

// cretae a socket
// bind the socket to IP / port
// mark the socket for listening
// accept a call
// close the listening socket
// 
// close socket

int main()
{
    Server server;
    ParseConfigeFile config;
    config.parser("./parse/configfile.txt");
    for (size_t i = 0; i < config.getData().size(); i++)
    {
        server.createSocket(config.getData()[i]);
    }
    
    server.createServer(config.getData());
    server.putServerOnListening();

    std::vector<int> activeConnections;
    fd_set setOfFds, readSet, writeSet;
    FD_ZERO(&setOfFds);
    int max_fd = -1;
    for (size_t i = 0; i < server.getServerSockets().size(); i++)
    {
        int fd = server.getServerSocket(i);
        FD_SET(fd, &setOfFds);
        if (fd > max_fd)
            max_fd = fd;
    }
 
    while (true) 
    {
        readSet = setOfFds;
        writeSet = setOfFds;
        int selected = select(max_fd + 1, &readSet, &writeSet, NULL, NULL);
        if (selected == -1)
        {
            std::cerr << "Error in select\n";
            return 1;
        }
        for (size_t i = 0; i < server.getServerSockets().size(); i++)
        {
            if (FD_ISSET(server.getServerSocket(i), &readSet))
            {
                int clientSocket = accept(server.getServerSocket(i), NULL, NULL);
                if (clientSocket == -1)
                {
                    std::cerr << "Error accepting client connection\n";
                }
                else
                {
                    int flags = fcntl(clientSocket, F_GETFL, 0);
                    if (flags == -1) {
                        std::cerr << "Error getting flags for socket\n";
                        return 1;
                    }
                    // std::cout << "*********************************>flags : " << flags << "\n";
                    flags |= O_NONBLOCK;
                    if (fcntl(clientSocket, F_SETFL, flags) == -1) {
                        std::cerr << "Error setting socket to non-blocking\n";
                        return 1;
                    }
                    
                    std::cout << "New client connected, socket: " << clientSocket << "\n";
                    if (clientSocket > max_fd)
                        max_fd = clientSocket;
                    // std::cout << "max = " << max_fd << "\n";
                    activeConnections.push_back(clientSocket);
                    FD_SET(clientSocket, &setOfFds);
                }
            }
        }
        char buffer[4096];
        for(std::vector<int>::iterator it = activeConnections.begin(); it != activeConnections.end(); ++it)
        {
            if (FD_ISSET(*it, &readSet))
            {
                memset(buffer, 0, 4096);
                ssize_t bytesRead = recv(*it, buffer, 4096 - 1, 0); //receive request
                buffer[bytesRead] = '\0';
                if (bytesRead > 0)
                {
                    std::cout << "Socket : " << *it << " Received request:\n" << buffer << std::endl;
                    if (FD_ISSET(*it, &writeSet))
                    {
                        std::string request(buffer);
                        std::string method = request.substr(0, request.find(" "));
                        std::cout << "HTTP method: " << method << std::endl;
                        if (method == "GET")
                        {
                            size_t firstSpace = request.find(" ");
                            size_t secondSpace = request.find(" ", firstSpace + 1);
                            std::string target = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                            std::cout << "---------->|" << target << "|\n";
                            // if (target == "/home")
                            // {
                                std::cout << "===================>" << *it << " is ready to write\n";
                                std::ostringstream ss;
                                std::ifstream file("index.html");
                                if (!file)
                                {
                                    std::cerr << "Error opening file" << std::endl;
                                    return 1;
                                }
                                ss << file.rdbuf();
                                std::string file_contents = ss.str();
                                std::ostringstream response;
                                response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << file_contents.size() << "\r\n\r\n" << file_contents;
                                // Send the HTTP response
                                send(*it, response.str().c_str(), response.str().size(), 0);
                                // close(*it);
                            // }
                        }
                    }
                }
                else if (bytesRead == 0)
                {
                    // Connection closed by the client
                    std::cout << "Client disconnected, socket: " << *it << "\n";
                    close(*it);
                    FD_CLR(*it, &setOfFds);
                    it = activeConnections.erase(it) - 1; // Decrement iterator to avoid skipping the next element
                }
                else
                {
                    // Error in recv
                    std::cerr << "Error receiving data from client, socket: " << *it << "\n";
                    close(*it);
                    FD_CLR(*it, &setOfFds);
                    it = activeConnections.erase(it) - 1;
                }
            }
        }
    }
}