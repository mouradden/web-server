#include "Server.hpp"

void Server::createSocket(DataConfige config)
{
    std::vector<std::string> ports = config.getListen();
    // std::cout << "size = " << ports.size() << "\n";
    for (size_t i = 0; i < ports.size(); i++)
    {
        int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        // std::cout << " i = " << i << " socketFd = " << socketFd << "\n";
        if (socketFd == -1)
        {
            std::cout << "Failed to create socket. Exiting..." << std::endl;
            exit(1);
        }
        std::cout << "[INFO] Socket successfully created" << std::endl;
        int enable = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            std::cerr << "setsockopt(SO_REUSEADDR) failed";
        serverSockets.push_back(socketFd);
    }
}

void Server::createServer(DataConfige config)
{
    std::vector<std::string> ports = config.getListen();
    // std::vector<sockaddr_in> serverAddress;
    for (size_t i = 0; i < ports.size(); i++)
    {
        // std::cout << " i = " << i << " port = " << ports[i] << "\n";
        sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET; // address family : ipv4
        address.sin_addr.s_addr = INADDR_ANY;
        // address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // specify the IP address to which the server will bind
        // address.sin_addr.s_addr = inet_addr("127.0.0.1"); // specify the IP address to which the server will bind
        // std::cout << "port : " << *(serverSockets.begin() + i) << "\n";
        address.sin_port = htons(std::stoi(ports[i]));
        serverAddress.push_back(address);
    }
        // std::cout << " ii = " << ports.size() << "\n";
        // std::cout << "socket0 = " << serverSockets[0] << "\n";
        // std::cout << "socket1 = " << serverSockets[1] << "\n";
    for (size_t i = 0; i < serverSockets.size(); i++)
    {
        int binded = bind(this->serverSockets[i], reinterpret_cast<sockaddr*>(&(this->serverAddress[i])), sizeof(this->serverAddress[i]));
        // std::cout << "bind = " << binded << "\n";
        if (binded == -1)
        {
            std::cout << "Failed to bind the socket.to port " << ports[i] << ". Exiting..." << std::endl;
            close(this->serverSockets[i]);
            exit(1);
        }
        std::cout << "[INFO] Server created : socket successfully binded with 127.0.0.1:" << ports[i] << std::endl;
    }
}

void Server::putServerOnListening(DataConfige config)
{
    std::vector<std::string> ports = config.getListen();
    for (size_t i = 0; i < ports.size(); i++)
    {
        int listened = listen(serverSockets[i], 5);
        // std::cout << "listen = " << listened << "\n";
        if (listened == -1)
        {
            std::cout << "Failed to put the server on listening on port " << ports[i] << " Exiting..." << std::endl;
            close(serverSockets[i]);
            exit(1);
        }
        std::cout << "[INFO] Server listening on port " << ports[i] << std::endl;
    }
}

const std::vector<int>& Server::getServerSocket()
{
    return serverSockets;
}
const std::vector<sockaddr_in>& Server::getServerAddress()
{
    return serverAddress;
}