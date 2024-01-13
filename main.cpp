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

    // putting the server socket into the listening state
    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Error listening for connections\n";
        // close(serverSocket);
        // return EXIT_FAILURE;
    }
    std::cout << "Server listening on port " << 8080 << std::endl;

    fd_set master_set, read_set;
FD_ZERO(&master_set);
FD_SET(serverSocket, &master_set);

int max_fd = serverSocket;

while (true) 
{
    read_set = master_set;

    if (select(max_fd + 1, &read_set, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(4);
    }

    for(int fd = 0; fd <= max_fd; fd++)
    {
        if (FD_ISSET(fd, &read_set)) // fd is ready for reading
        {
            if (fd == serverSocket) // request for new connection
            {
                // handle new connections
                sockaddr_in clientAddress;
                socklen_t clientAddressSize = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);

                if (clientSocket == -1)
                {
                    perror("accept");
                } else
                {
                    FD_SET(clientSocket, &master_set); // add to master set
                    if (clientSocket > max_fd)
                    {    // keep track of the max
                        max_fd = clientSocket;
                    }
                    printf("New connection from %s on socket %d\n", inet_ntoa(clientAddress.sin_addr), clientSocket);
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
                        printf("Socket %d hung up\n", fd);
                    } 
                    else
                    {
                        perror("recv");
                    }
                    close(fd); // bye!
                    FD_CLR(fd, &master_set); // remove from master set
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