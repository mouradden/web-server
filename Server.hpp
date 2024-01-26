#pragma once

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>

class Server
{
    public :
        int serverSocket;
        sockaddr_in serverAddress;
    public :
        void createSocket();
        void createServer();
        void putServerOnListening();

        const int& getServerSocket();
        const sockaddr_in& getServerAddress();
};