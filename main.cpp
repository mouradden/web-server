#include "Server.hpp"
#include <fstream>
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
int sendResponse(int socket, Client& client)
{
    size_t totalSize = client.getResponseBuffer().size();

    if (client.getSentOffset() < totalSize) {
        ssize_t sendResult = send(socket, client.getResponseBuffer().c_str() + client.getSentOffset(), totalSize - client.getSentOffset(), 0);
        if (sendResult == -1) {
            std::cerr << "Error sending data\n";
            return -1;
        }
        client.incremetOffset(sendResult);
        std::cout << "socket = " << socket << "  ------ data sent : " << client.getSentOffset() << " / " << client.getResponseBuffer().size() << "\n";
        return 1;
    }
    return 0;
}

void parseChunkedRequest(std::string& requestBuffer) {
    std::string buffer;
    size_t pos = 0;

    pos = requestBuffer.find("\r\n\r\n");
    if (pos == std::string::npos) {
        std::cerr << "Error: Couldn't find end of headers" << std::endl;
        return;
    }

    buffer += requestBuffer.substr(0, pos + 4);
    pos += 4; // Move past the end of headers

    while (true) {
        if (pos >= requestBuffer.size())
            break;
         if (!isdigit(requestBuffer[pos]) && !(requestBuffer[pos] <= 'f' && requestBuffer[pos] >= 'a'))
            break ;
        size_t chunkSizePos = requestBuffer.find("\r\n", pos);
        if (chunkSizePos == std::string::npos) {
            std::cerr << "Error: Couldn't find chunk size" << std::endl;
            return;
        }

        int chunkSize;
        std::istringstream(requestBuffer.substr(pos, chunkSizePos - pos)) >> std::hex >> chunkSize;
        std::cout << "length = " << chunkSizePos - pos << " size = " <<  chunkSize << "\n";
        if (chunkSize <= 0) {
            // End of chunks
            break;
        }

        buffer += requestBuffer.substr(chunkSizePos + 2, chunkSize);
        pos = chunkSizePos + 2 + chunkSize; // 2 for CRLF, additional 2 for next CRLF
        if (pos + 1 < requestBuffer.size() && requestBuffer[pos] == '\r' && requestBuffer[pos + 1] == '\n')
        {
            pos += 2;
        }
    }
    requestBuffer = buffer;
}
int main()
{
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
    // std::vector<int> fdsToRemove;
    while (true) 
    {
        int pollResult = poll(fds.data(), fds.size(), 1000);
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
                    DataConfig config = server.getServers()[clientSocket];
                    Clients[clientSocket].getRequestBuffer().append(buffer, bytesRead);
                    if (Clients[clientSocket].getRequestBuffer().find("Transfer-Encoding: chunked") != std::string::npos)
                    {
                        // std::cout << "********************chunked*******\n";
                        // Clients[clientSocket].getRequestBuffer().append(buffer, bytesRead);
                        // std::cout << "--------------> request before : |" << GREEN << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                        if (Clients[clientSocket].getRequestBuffer().find("\r\n0") != std::string::npos)
                        {
                            parseChunkedRequest(Clients[clientSocket].getRequestBuffer());
                        // std::cout << "--------------> request after : |" << RED << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                            Request req(Clients[clientSocket].getRequestBuffer());
                            Response response = req.handleRequest(config);
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
                            // std::cout << "--------------> request before : \n|" << GREEN << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
                            Request req(Clients[clientSocket].getRequestBuffer());
                            Response response = req.handleRequest(config);
                            Clients[clientSocket].setResponse(response.getResponseEntity());
                            fds[i].events |= POLLOUT;
                            fds[i].events &= ~POLLIN;
                        }
                    }
                } 
                else if (bytesRead == 0)
                {
                    std::cout << "Client disconnected, socket: " << clientSocket << "\n";
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
                if (sendResponse(clientSocket, Clients[clientSocket]) == 0)
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

        // for (size_t i = 0; i < fds.size(); i++)
        // {
        //     if (fds[i].revents & POLLIN)
        //     {
        //         std::vector<int> sockets = server.getServerSockets();
        //         std::vector<int>::iterator it = std::find(sockets.begin(), sockets.end(), fds[i].fd);
        //         if (it != sockets.end() && fds[i].fd == *it)
        //         {
        //             int clientSocket = accept(fds[i].fd, NULL, NULL);
        //             if (clientSocket == -1)
        //             {
        //                 std::cerr << "Error accepting client connection\n";
        //             }
        //             else
        //             {
        //                 if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1) {
        //                     std::cerr << "Error setting socket to non-blocking\n";
        //                     return 1;
        //                 }
                        
        //                 std::cout << "New client connected, socket: " << clientSocket << "\n";
        //                 activeConnections.push_back(clientSocket);
        //                 Clients.insert(std::make_pair(clientSocket, Client()));
        //                 pollfd pfd;
        //                 pfd.fd = clientSocket;
        //                 pfd.events = POLLIN;
        //                 fds.push_back(pfd);
        //                 server.setServer(clientSocket, server.getServers()[fds[i].fd]);
        //             }
        //         }
        //         else
        //         {
        //             int clientSocket = fds[i].fd;
        //             char buffer[4096];
        //             ssize_t bytesRead = recv(clientSocket, buffer, 4096 - 1, 0);
        //             if (bytesRead > 0)
        //             {
        //                 std::cout << "Handling request for socket: " << clientSocket << std::endl;
        //                 DataConfig config = server.getServers()[clientSocket];
        //                 Request req(buffer);
        //                 Response response = req.handleRequest(config);
                        
        //                 Clients[clientSocket].getRequestBuffer().clear();
        //                 Clients[clientSocket].getResponseBuffer().clear();
        //                 Clients[clientSocket].setOffset(0);

        //                 Clients[clientSocket].setRequest(buffer);
        //                 Clients[clientSocket].setResponse(response.getResponseEntity());
        //                 fds[i].events |= POLLOUT;
        //                 fds[i].events &= ~POLLIN;
        //             } 
        //             else if (bytesRead == 0)
        //             {
        //                 std::cout << "Client disconnected, socket: " << clientSocket << "\n";
        //                 close(clientSocket);
        //                 fds.erase(fds.begin() + i);
        //                 // fdsToRemove.push_back(i);
        //                 Clients.erase(clientSocket);
        //                 activeConnections.erase(std::remove(activeConnections.begin(), activeConnections.end(), clientSocket), activeConnections.end());
        //             }
        //             else
        //             {
        //                 std::cerr << "recv return  = " << bytesRead << "\n";
        //                 std::cerr << "Error receiving data from client, socket: " << clientSocket << "\n";
        //                 close(clientSocket);
        //                 fds.erase(fds.begin() + i);
        //                 // fdsToRemove.push_back(i);
        //                 Clients.erase(clientSocket);
        //                 activeConnections.erase(std::remove(activeConnections.begin(), activeConnections.end(), clientSocket), activeConnections.end());
        //             }
        //         }
        //     }
        // }
// #include "Server.hpp"
// #include <iostream>
// #include <vector>
// #include <list>
// #include "httpstuff/Request.hpp"
// #include "httpstuff/Response.hpp"
// #include "parse/DataConfig.hpp"

// int sendResponse(int socket, Client& client)
// {
//     size_t totalSize = client.getResponseBuffer().size();

//     if (client.getSentOffset() < totalSize) {
//         ssize_t sendResult = send(socket, client.getResponseBuffer().c_str() + client.getSentOffset(), totalSize - client.getSentOffset(), 0);
//         if (sendResult == -1) {
//             std::cerr << "Error sending data\n";
//             return -1;
//         }
//         client.incremetOffset(sendResult);
//         std::cout << "date sent : " << client.getSentOffset() << "\n";
//         return 1;
//     }
//     return 0;
// }

// int main()
// {
//     Server server;
//     ParseConfigFile config;
//     config.parser("./parse/configfile.txt");
//     for (size_t i = 0; i < config.getData().size(); i++)
//     {
//         server.createSocket(config.getData()[i]);
//     }
    
//     server.createServer(config.getData());
//     server.putServerOnListening();

//     std::vector<int> activeConnections;
//     fd_set setOfFds, readSet, writeSet;
//     FD_ZERO(&setOfFds);
//     FD_ZERO(&readSet);
//     FD_ZERO(&writeSet);
//     int max_fd = -1;
//     for (size_t i = 0; i < server.getServerSockets().size(); i++)
//     {
//         int fd = server.getServerSocket(i);
//         FD_SET(fd, &setOfFds);
//         if (fd > max_fd)
//             max_fd = fd;
//     }

//     std::map<int, Client> Clients;
//     struct timeval timeout;
//     timeout.tv_sec = 5;
//     timeout.tv_usec = 0;
//     while (true) 
//     {
//         readSet = setOfFds;
//         int selectResult = select(max_fd + 1, &readSet, &writeSet, NULL, &timeout);
//         if (selectResult == -1)
//         {
//             std::cerr << "Error in select\n";
//             return 1;
//         }
//         // else if (selectResult == 0)
//         // {
//         //     std::cout << "Timeout occurred\n";
//         //     // Clearing and resetting the sets and timeout for the next iteration
//         //     FD_ZERO(&readSet);
//         //     FD_ZERO(&writeSet);
//         //     FD_ZERO(&setOfFds);
//         //     for (size_t i = 0; i < server.getServerSockets().size(); i++)
//         //     {
//         //         int fd = server.getServerSocket(i);
//         //         FD_SET(fd, &setOfFds);
//         //         if (fd > max_fd)
//         //             max_fd = fd;
//         //     }
//         //     continue;
//         // }

//         for (size_t i = 0; i < server.getServerSockets().size(); i++)
//         {
//             if (FD_ISSET(server.getServerSocket(i), &readSet))
//             {
//                 int clientSocket = accept(server.getServerSocket(i), NULL, NULL);
//                 if (clientSocket == -1)
//                 {
//                     std::cerr << "Error accepting client connection\n";
//                 }
//                 else
//                 {
//                     if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1) {
//                         std::cerr << "Error setting socket to non-blocking\n";
//                         return 1;
//                     }
                    
//                     std::cout << "New client connected, socket: " << clientSocket << "\n";
//                     if (clientSocket > max_fd)
//                         max_fd = clientSocket;
//                     activeConnections.push_back(clientSocket);
//                     Client c;
//                     Clients.insert(std::make_pair(clientSocket, c));
//                     FD_SET(clientSocket, &setOfFds);
//                     // FD_SET(clientSocket, &readSet);
//                     server.setServer(clientSocket, server.getServers()[server.getServerSocket(i)]);
//                 }
//             }
//         }

//         // for(std::vector<int>::iterator it = activeConnections.begin(); it != activeConnections.end(); it++)
//         // {
//         std::vector<int>::iterator it = activeConnections.begin();
//         while (it != activeConnections.end())
//         {
//             // std::find(activeConnections.begin(), activeConnections.end(), *it) != activeConnections.end() && 
//             if (FD_ISSET(*it, &readSet) && !FD_ISSET(*it, &writeSet))
//             {
//                 char buffer[4096];
//                 memset(buffer, 0, 4096);
//                 ssize_t bytesRead = recv(*it, buffer, 4096 - 1, 0); //receive request
//                 if (bytesRead > 0)
//                 {
//                     std::cout << "Handling request for socket: " << *it << std::endl;
//                     DataConfig config = server.getServers()[*it];
//                     Request req(buffer);
//                     Response response = req.handleRequest(config);
                    
//                     Clients[*it].getRequestBuffer().clear();
//                     Clients[*it].getResponseBuffer().clear();
//                     Clients[*it].setOffset(0);

//                     Clients[*it].setRequest(buffer);
//                     Clients[*it].setResponse(response.getResponseEntity());
//                     FD_SET(*it, &writeSet);
//                     FD_CLR(*it, &readSet);
//                     // it++;
//                 } 
//                 else if (bytesRead == 0)
//                 {
//                     std::cout << "Client disconnected, socket: " << *it << "\n";
//                     // close(*it);
//                     Clients[*it].getRequestBuffer().clear();
//                     Clients[*it].getResponseBuffer().clear();
//                     Clients[*it].setOffset(0);
//                     Clients.erase(*it);
//                     // FD_CLR(*it, &setOfFds);
//                     FD_CLR(*it, &readSet);
//                     FD_CLR(*it, &writeSet);
//                     it = activeConnections.erase(it);
//                     // it++;
//                     continue;
//                 }
//                 else
//                 {
//                     std::cerr << "recv return  = " << bytesRead << "\n";
//                     std::cerr << "Error receiving data from client, socket: " << *it << "\n";
//                     // close(*it);
//                     Clients[*it].getRequestBuffer().clear();
//                     Clients[*it].getResponseBuffer().clear();
//                     Clients[*it].setOffset(0);
//                     Clients.erase(*it);
//                     // FD_CLR(*it, &setOfFds);
//                     FD_CLR(*it, &readSet);
//                     FD_CLR(*it, &writeSet);
//                     it = activeConnections.erase(it);
//                     // it++;
//                     continue;
//                 }
//             }
//             else
//                 it++;
//         }
//         // for(std::vector<int>::iterator it = activeConnections.begin(); it != activeConnections.end(); it++)
//         // {
//         it = activeConnections.begin();
//         while (it != activeConnections.end())
//         {
//             if (!FD_ISSET(*it, &readSet) && FD_ISSET(*it, &writeSet))
//             {
//                 std::cout << "Handling response for socket: " << *it << std::endl;
//                 std::cout << "total response : " << Clients[*it].getResponseBuffer().size() << "\n";
//                 if (sendResponse(*it, Clients[*it]) == 0)
//                 {
//                     std::cout << "Response sent for socket: " << *it << "\n";
//                     Clients[*it].getRequestBuffer().clear();
//                     Clients[*it].getResponseBuffer().clear();
//                     Clients[*it].setOffset(0);
//                     FD_CLR(*it, &writeSet);
//                     FD_SET(*it, &readSet);
//                     // close(*it);
//                     // FD_CLR(*it, &setOfFds);
//                     // Clients.erase(*it);
//                     // it = activeConnections.erase(it);
//                 }
//                 else
//                     it++;
//             }
//             else
//                 it++;
//         }
//     }
//     return 0;
// }
