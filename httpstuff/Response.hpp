#pragma once

#include "../Server.hpp"
#include "Client.hpp"
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
class Client;
class Response {
    private:
        std::string httpVersion;
        unsigned int code;
        std::string status;
        std::string contentType;
        unsigned int contentLength;
        std::map<std::string, std::string> headers;
        std::string body;
        std::string responseEntity;

        // chunked related stuff
        // int socket;
        // int state;
        // int offset;

    public:
        Response();
        void buildResponse(unsigned int code);
        int sendResponse(int socket, Client client);

        Response(const Response& ref);
        Response& operator=(const Response& ref);

        void setStatus(unsigned int code);
        void setContentType(std::string fileExtension);
        void setContentLength(unsigned int length);
        void setResponseBody(std::string content);
        void setHeader(std::string key, std::string value);
        void setSocket(int socket);
        void setState(int state);
        void setResponseEntity(std::string response);
        void setFileOffset(int offset);

        std::string getContentType();
        unsigned int getContentLength();
        std::string getResponseEntity();
        std::string getStatus();
        // int getSocket();
        // int getState();
        // int getFileOffset();
        std::string getMimeType(std::string fileExtension);
};