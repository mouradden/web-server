#include "Server.hpp"
#include "parse/DataConfig.hpp"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

void Server::createSocket(DataConfig config)
{
    std::vector<std::string> ports = config.getListen();
    size_t i;
    for (i = 0; i < ports.size(); i++)
    {
        int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd == -1)
        {
            std::cout << "Failed to create socket. Exiting..." << std::endl;
            exit(1);
        }
        if (fcntl(socketFd, F_SETFL, O_NONBLOCK) == -1)
        {
            std::cerr << "Error setting socket to non-blocking\n";
            return ;
        }
        int enable = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            std::cerr << "setsockopt(SO_REUSEADDR) failed";
        setServerSocket(socketFd);
        servers[socketFd] = config;
    }
    std::cout << "[INFO] " << i << " Sockets successfully created" << std::endl;
}

void Server::createServer(std::vector<DataConfig> config)
{
    for (size_t i = 0; i < config.size(); i++)
    {
        std::vector<std::string> ports = config[i].getListen();
        for (size_t j = 0; j < ports.size(); j++)
        {
            sockaddr_in address;
            std::memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET; // address family : ipv4
            // address.sin_addr.s_addr = INADDR_ANY;
            // address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // specify the IP address to which the server will bind
            // std::cout << "host :" << config[i].getHost().c_str() << "\n";
            // address.sin_addr.s_addr = inet_addr(config[i].getHost().c_str()); // specify the IP address to which the server will bind
            if (!strcmp(config[i].getHost().c_str(), "localhost"))
            {
                address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            else
            {
                // Resolve host to its IP address
                if (inet_pton(AF_INET, config[i].getHost().c_str(), &address.sin_addr) <= 0) {
                    std::cerr << "Invalid address or address not supported\n";
                    return ;
                }
            }
            address.sin_port = htons(atoi(ports[j].c_str()));
            serverAddress.push_back(address);
        }
    }
 
    for (size_t i = 0; i < serverSockets.size(); i++)
    {
        // std::cout << "BINDING before " << config[i].getHost() << ":" << ntohs(serverAddress[i].sin_port) << std::endl;
        int bindResult = bind(this->getServerSocket(i), reinterpret_cast<sockaddr*>(&(this->serverAddress[i])), sizeof(this->serverAddress[i]));
        if (bindResult == -1)
        {
            std::cout << "Failed to bind the socket to " << config[i].getHost() << ":" << ntohs(serverAddress[i].sin_port) << ". Ignoring it..." << std::endl;
            close(this->getServerSocket(i));
            this->serverSockets.erase(this->serverSockets.begin() + i);
            return ;
            // exit(1);
        }
        std::cout << "[INFO] Server created : socket successfully binded with " << config[i].getHost() << ":" << ntohs(serverAddress[i].sin_port) << std::endl;
    }
}

void Server::putServerOnListening()
{
    for (size_t i = 0; i < serverSockets.size(); i++)
    {
        int listenResult = listen(getServerSocket(i), SOMAXCONN);
        if (listenResult == -1) {
            std::cerr << "Failed to listen on socket for port " << ntohs(serverAddress[i].sin_port) << ". Exiting..." << std::endl;
            close(getServerSocket(i));
            this->serverSockets.erase(this->serverSockets.begin() + i);
            return ;
            // exit(1);
        }
        std::cout << "[INFO] Server listening on port " << ntohs(serverAddress[i].sin_port) << std::endl;
    }
}

const std::vector<int>& Server::getServerSockets()
{
    return serverSockets;
}
bool Server::isServerSocket(int socket)
{
    std::vector<int>::iterator it = std::find(serverSockets.begin(), serverSockets.end(), socket);
    if (it != serverSockets.end())
    {
        return true;
    }
    return false;
}
const std::vector<sockaddr_in>& Server::getServerAddress()
{
    return serverAddress;
}

const int& Server::getServerSocket(int index)
{
    return (this->serverSockets[index]);
}

void Server::setServerSocket(int socket)
{
    this->serverSockets.push_back(socket);
}

std::map<int, DataConfig>& Server::getServers()
{
    return (this->servers);
}

void Server::setServer(int socketFd, DataConfig config)
{
    this->servers[socketFd] = config;
}

void    Server::acceptNewConnections(std::vector<pollfd>& fds, std::vector<pollfd>& fdsTmp, std::map<int, Client>& Clients, size_t& i)
{
    if (fdsTmp[i].revents & POLLIN)
    {
        if (this->isServerSocket(fdsTmp[i].fd) == true)
        {
            int clientSocket = accept(fdsTmp[i].fd, NULL, NULL);
            if (clientSocket == -1)
            {
                std::cerr << "Error accepting client connection\n";
                close(fdsTmp[i].fd);
            }
            else
            {
                if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1) {
                    std::cerr << "Error setting socket to non-blocking\n";
                    return ;
                }
                
                std::cout << "New client connected, socket: " << clientSocket << "\n";
                Clients.insert(std::make_pair(clientSocket, Client()));
                pollfd pfd;
                pfd.fd = clientSocket;
                pfd.events = POLLIN | POLLOUT;
                fds.push_back(pfd);
                this->setServer(clientSocket, this->getServers()[fdsTmp[i].fd]);
            }
        }
    }
}


void    Server::handleClientInput(std::vector<pollfd>& fds, std::vector<pollfd>& fdsTmp, std::map<int, Client>& Clients, size_t& i)
{
    if ((fdsTmp[i].revents & POLLIN) && this->isServerSocket(fdsTmp[i].fd) == false)
    {
        int clientSocket = fdsTmp[i].fd;
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead > 0)
        {
            DataConfig configData = this->getServers()[clientSocket];
            Clients[clientSocket].getRequestBuffer().append(buffer, bytesRead);
            std::cout << "request buffer : |" << GREEN << Clients[clientSocket].getRequestBuffer() << RESET << "|\n";
            if (Clients[clientSocket].getRequestBuffer().find("Transfer-Encoding: chunked") != std::string::npos)
            {
                if (Clients[clientSocket].getRequestBuffer().find("\r\n0") != std::string::npos)
                {
                    this->parseChunkedRequest(Clients[clientSocket].getRequestBuffer());
                    Request req(Clients[clientSocket].getRequestBuffer());
                    Response response = req.handleRequest(configData);
                    Clients[clientSocket].setResponse(response.getResponseEntity());
                }
            }
            else
            {
                if (Clients[clientSocket].getRequestBuffer().find("\r\n\r\n") != std::string::npos)
                {
                    
                    Request req(Clients[clientSocket].getRequestBuffer());
                    Response response = req.handleRequest(configData);
                    Clients[clientSocket].setResponse(response.getResponseEntity());
                    Clients[clientSocket].served = 0;
                }
            }
        } 
        else if (bytesRead == 0)
        {
            std::cout << "Client disconnected, socket: " << clientSocket << "\n";
            close(clientSocket);
            for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end();) {
                if (it->fd == clientSocket)
                    it = fds.erase(it);
                else
                    ++it;
            }
            Clients.erase(clientSocket);
        }
        else
        {
            close(clientSocket);
            for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); )
            {
                if (it->fd == clientSocket)
                {
                    it = fds.erase(it);
                } else {
                    ++it;
                }
            }
            Clients.erase(clientSocket);
        }
    }
}

void    Server::deliverResponseToClient(std::vector<pollfd>& fds, std::vector<pollfd>& fdsTmp, std::map<int, Client>& Clients, size_t& i)
{
    if ((fdsTmp[i].revents & POLLOUT) && this->isServerSocket(fdsTmp[i].fd) == false)
        {
        int clientSocket = fdsTmp[i].fd;
        if (this->sendResponse(clientSocket, Clients[clientSocket]) == 0 && !Clients[clientSocket].served)
        {
            std::cout << "Response sent for socket: " << clientSocket << "\n";
            close(clientSocket);
            for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end();) {
                if (it->fd == clientSocket)
                {
                    it = fds.erase(it);
                } else {
                    ++it;
                }
            }
            Clients[clientSocket].getRequestBuffer().clear();
            Clients[clientSocket].getResponseBuffer().clear();
            Clients[clientSocket].setOffset(0);
            Clients[clientSocket].served = 1;
        }
    }
}

int Server::sendResponse(int socket, Client& client)
{
    // std::cerr << "response = "  << RED << client.getResponseBuffer() << RESET << "\n";
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

void Server::parseChunkedRequest(std::string& requestBuffer) {
    std::string buffer;
    size_t pos = 0;

    pos = requestBuffer.find("\r\n\r\n");
    if (pos == std::string::npos) {
        std::cerr << "Error: Couldn't find end of headers" << std::endl;
        return;
    }

    buffer += requestBuffer.substr(0, pos + 4);
    pos += 4; // Move past the end of headers

    while (true)
    {
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

