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


DataConfig  findConfig(ParseConfigFile configFile, std::string serverName, std::string host, std::string port)
{
   (void)host; (void)port;
    std::vector<DataConfig> config = configFile.getData();
    for (size_t i = 0; i < config.size(); i++)
    {
        DataConfig currentConfig = config[i];
        std::vector<std::string> servers = currentConfig.getServerName();
        for(std::vector<std::string>::iterator it = servers.begin(); it != servers.end(); ++it)
        {
            if ((*it).compare(serverName) == 0)
            {
                // std::cout << "server = |" << *it << "| " << serverName << "\n";
                return (currentConfig);
            }
        }
    }
    for (size_t i = 0; i < config.size(); i++)
    {
        int count = 0;
        DataConfig currentConfig = config[i];
        if (currentConfig.getHost().compare(host) == 0)
            count += 1;
        std::vector<std::string> ports = currentConfig.getListen();
        for (size_t j = 0; j < ports.size(); j++)
        {
            if (ports[j].compare(port) == 0)
            {
                count += 1;
                break;
            }
        }
        if (count == 2)
            return (currentConfig);
    }
    return (DataConfig());
}

int main(int ac, char **av, char **envp)
{
    (void)ac;(void)av;(void)envp;
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
    std::vector<pollfd> fdsTmp;
    for (size_t i = 0; i < server.getServerSockets().size(); i++)
    {
        int fd = server.getServerSocket(i);
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN | POLLOUT;
        pfd.revents = 0;
        fds.push_back(pfd);
    }
    while (true) 
    {
        // DataConfig conf = findConfig(config, "googl1e.com", "127.0.0.1", "8081");
        // conf.printDataConfig();
        // exit(1);
        fdsTmp = fds;
        int pollResult = poll(fdsTmp.data(), fdsTmp.size(), 0);
        if (pollResult == -1)
        {
            std::cerr << "Error in poll\n";
            return 1;
        }
        for (size_t i = 0; i < fdsTmp.size(); i++)
        {
            server.acceptNewConnections(fds, fdsTmp, Clients, i);
            server.handleClientInput(fds, fdsTmp, Clients, i);
            server.deliverResponseToClient(fds, fdsTmp, Clients, i);
        }
    }
    return 0;
}