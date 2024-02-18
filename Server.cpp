#include "Server.hpp"

void Server::createSocket()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cout << "Failed to create socket. Exiting..." << std::endl;
        exit(1);
    }
    std::cout << "[INFO] Socket successfully created" << std::endl;
    int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        std::cerr << "setsockopt(SO_REUSEADDR) failed";
}

void Server::createServer()
{
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET; // address family : ipv4
    // serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // specify the IP address to which the server will bind
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1)
    {
        std::cout << "Failed to bind the socket. Exiting..." << std::endl;
        close(serverSocket);
        exit(1);
    }
    std::cout << "[INFO] Server created : socket successfully binded with 127.0.0.1:8080"  << std::endl;
}

void Server::putServerOnListening()
{
    if (listen(serverSocket, 5) == -1)
    {
        std::cout << "Failed to put the server on listening. Exiting..." << std::endl;
        close(serverSocket);
        exit(1);
    }
    std::cout << "[INFO] Server listening on port " << 8080 << std::endl;
}

const int& Server::getServerSocket()
{
    return serverSocket;
}
const sockaddr_in& Server::getServerAddress()
{
    return serverAddress;
}