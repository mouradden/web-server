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

    server.createSocket();
    server.createServer();
    server.putServerOnListening();

    fd_set setOfFds, readSet;
    FD_ZERO(&setOfFds);
    FD_SET(server.getServerSocket(), &setOfFds);

    int max_fd = server.getServerSocket();

    while (true) 
    {
        readSet = setOfFds;

        if (select(max_fd + 1, &readSet, NULL, NULL, NULL) == -1)
        {
            std::cerr << "select";
            exit(1);
        }
        if (FD_ISSET(server.getServerSocket(), &readSet))
        {
            sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);
            int clientSocket = accept(server.getServerSocket(), reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize); // return the new socket fd for the incoming client connection
            if (clientSocket != -1)
            {
                FD_SET(clientSocket, &setOfFds); // add to global set
                if (clientSocket > max_fd)   // update the max
                {
                    max_fd = clientSocket;
                }
                std::cout << "New connection from " << inet_ntoa(clientAddress.sin_addr) << " on socket " << clientSocket << "\n"  ;

                
            }
        }
        for(int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &readSet)) // fd is ready for reading
            {
                char buffer[4096];
                memset(buffer, 0, 4096);
                ssize_t bytesRead = recv(fd, buffer, 4096 - 1, 0);
                if (bytesRead > 0)
                {
                    std::cout << "Received request:\n" << buffer << std::endl;
                    std::ifstream file("index.html");
                    if (!file)
                    {
                        std::cerr << "Error opening file" << std::endl;
                        return 1;
                    }
                    // Read the file into a string
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    std::string file_contents = ss.str();
                    // Prepare the HTTP response
                    std::ostringstream response;
                    response << "HTTP/1.1 200 OK\r\nContent-Length: " << file_contents.size() << "\r\n\r\n" << file_contents;
                    // Send the HTTP response
                    send(fd, response.str().c_str(), response.str().size(), 0);
                }
                else if (bytesRead == 0)
                {
                    std::cout << "Client closed connection" << std::endl;
                }
                else
                {
                    std::cerr << "Error reading from socket" << std::endl;
                    close(fd);
                    FD_CLR(fd, &setOfFds); // remove from master set
                }
            }
        }
    }
}