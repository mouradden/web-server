#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>

// cretae a socket
// bind the socket to IP / port
// mark the socket for listening
// accept a call
// close the listening socket
// 
// close socket
void handleConnection(int clientSocket)
{
    char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, World!";
    send(clientSocket, response, sizeof(response), 0);
    close(clientSocket);
}
int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket\n";
        return EXIT_FAILURE;
    }
    std::cout << "[INFO] Socket successfully created" << std::endl;

    // associates the socket with the address and port
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET; // address family : ipv4
    // serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // specify the IP address to which the server will bind
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1)
    {
        std::cerr << "Error binding socket\n";
        close(serverSocket);
        return EXIT_FAILURE;
    }
    std::cout << "[INFO] Socket successfully binded with 127.0.0.1:8080"  << std::endl;
    // putting the server socket into the listening state
    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Error listening for connections\n";
        // close(serverSocket);
        // return EXIT_FAILURE;
    }
    std::cout << "[INFO] Server listening on port " << 8080 << std::endl;

    fd_set setOfFds, readSet;
    FD_ZERO(&setOfFds);
    FD_SET(serverSocket, &setOfFds);

    int max_fd = serverSocket;

    while (true) 
    {
        readSet = setOfFds;

        if (select(max_fd + 1, &readSet, NULL, NULL, NULL) == -1) {
            std::cerr << "select";
            exit(4);
        }
    for(int fd = 0; fd <= max_fd; fd++)
    {
        if (FD_ISSET(fd, &readSet)) // fd is ready for reading
        {
            if (fd == serverSocket) // request for new connection
            {
                // handle new connections
                sockaddr_in clientAddress;
                socklen_t clientAddressSize = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize); // return the new socket fd for the incoming client connection

                if (clientSocket == -1)
                {
                    std::cerr << "accept";
                }
                else
                {
                    FD_SET(clientSocket, &setOfFds); // add to global set
                    if (clientSocket > max_fd)   // update the max
                    {
                        max_fd = clientSocket;
                    }
                    std::cout << "New connection from " << inet_ntoa(clientAddress.sin_addr) << "on socket " << clientSocket << "\n"  ;
                }
            }
            else
            {
                // handle data from a client
                char buffer[256];
                int nbytes = recv(fd, buffer, sizeof buffer, 0);

                if (nbytes <= 0)
                {
                    // got error or connection closed by client
                    if (nbytes == 0) {
                        // connection closed
                        std::cout << "Socket " << fd << " hung up\n";
                    } 
                    else
                    {
                        std::cerr << "recv";
                    }
                    close(fd); // bye!
                    FD_CLR(fd, &setOfFds); // remove from master set
                }
                else 
                {
                    // we got some data from a client
                    // char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, World!";
                    // send(i, response, sizeof(response), 0);
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
            } 
        }
    }
    // while (true) 
    // {
    //     // Accept a connection
    //     sockaddr_in clientAddress;
    //     socklen_t clientAddressSize = sizeof(clientAddress);
    //     int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);

    //     if (clientSocket == -1) {
    //         std::cerr << "Error accepting connection\n";
    //         close(serverSocket);
    //         return EXIT_FAILURE;
    //     }

    //     std::cout << "Connection accepted from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << std::endl;

    //      if (fork() == 0)
    //      {
    //         close(serverSocket);
    //         handleConnection(clientSocket);
    //         exit(EXIT_SUCCESS);
    //     }
    //     else
    //     {
    //         close(clientSocket);
    //     }

    }
}