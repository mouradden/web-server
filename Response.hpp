#pragma once

#include "Server.hpp"
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

class Response {
    private:
        std::string res;
        std::string httpVersion;
        unsigned int code;
        std::string status;

    public:
        Response(std::string res, std::string httpVersion, unsigned int code, std::string status);
        void buildResponse(std::string fileContent, std::string fileType);
        void sendResponse(int socket);

        Response(const Response& ref);
        Response& operator=(const Response& ref);
};