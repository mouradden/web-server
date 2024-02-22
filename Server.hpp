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
#include <map>

class Server
{
    private:
        std::vector<int> serverSockets;
        std::map<int, DataConfige> servers;
        std::vector<sockaddr_in> serverAddress;

    public :
        void createSocket(DataConfige config);
        void createServer(std::vector<DataConfige> config);
        void putServerOnListening();

        const std::vector<int>& getServerSockets();
        const int& getServerSocket(int index);
        void setServerSocket(int socket);
        const std::vector<sockaddr_in>& getServerAddress();

        std::map<int, DataConfige>& getServers();
};