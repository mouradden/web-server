#include "Server.hpp"
#include <vector>
#include "Request.hpp"
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

    server.createSocket();
    server.createServer();
    server.putServerOnListening();

    std::vector<int> activeConnections;
    fd_set setOfFds, readSet;
    FD_ZERO(&setOfFds);
    FD_SET(server.getServerSocket(), &setOfFds);
    int max_fd = server.getServerSocket();
 

    while (true) 
    {
        readSet = setOfFds;

        if (select(max_fd + 1, &readSet, NULL, NULL, NULL) == -1)
        {
            std::cerr << "Error in select\n";
            return 1;
        }
        if (FD_ISSET(server.getServerSocket(), &readSet))
        {
            int clientSocket = accept(server.getServerSocket(), NULL, NULL);
            if (clientSocket == -1)
            {
                std::cerr << "Error accepting client connection\n";
            }
            else
            {
                std::cout << "New client connected, socket: " << clientSocket << "\n";
                if (clientSocket > max_fd)
                    max_fd = clientSocket;
                activeConnections.push_back(clientSocket);
                FD_SET(clientSocket, &setOfFds);
            }
        }
            for(std::vector<int>::iterator it = activeConnections.begin(); it != activeConnections.end(); ++it)
            {
                if (FD_ISSET(*it, &readSet))
                {
                    char buffer[4096];
                    memset(buffer, 0, 4096);
                    ssize_t bytesRead = recv(*it, buffer, 4096 - 1, 0); //receive request
                    if (bytesRead > 0)
                    {
                        // std::cout << "Socket : " << *it << " Received request:\n" << buffer << std::endl;
                        // example how to handle a request
                        // std::string request(buffer);
                        // std::string method = request.substr(0, request.find(" "));
                        // std::cout << "HTTP method: " << method << std::endl;
                        Request req(buffer);
                        if (req.getRequestMethod() == "GET")
                        {
                            
                            if (req.getRequestRessource() == "/")
                            {
                                std::ostringstream ss;
                                std::ifstream file("index.html");
                                if (!file)
                                {
                                    std::cerr << "Error opening file" << std::endl;
                                    return 1;
                                }
                                ss << file.rdbuf();
                                std::string file_contents = ss.str();
                                // // Prepare the HTTP response
                                std::ostringstream response;
                                // std::cout << "file_contents : " << file_contents << std::endl;
                                response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << file_contents.size() << "\r\n\r\n" << file_contents;
                                // Send the HTTP response
                                send(*it, response.str().c_str(), response.str().size(), 0);
                                // close(*it);
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