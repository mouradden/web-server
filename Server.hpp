#pragma once


#include "./parse/DataConfige.hpp"
#include "./parse/ParseConfigeFile.hpp"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>

#include <vector>

class Server
{
    public :
        std::vector<int> serverSockets;
        std::vector<sockaddr_in> serverAddress;
    public :
        void createSocket(DataConfige config);
        void createServer(DataConfige config);
        void putServerOnListening(DataConfige config);

        const std::vector<int>& getServerSocket();
        const std::vector<sockaddr_in>& getServerAddress();
};