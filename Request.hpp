#include "Server.hpp"
#include "Response.hpp"
#include <map>
#include <algorithm>

class Request {
    private:
        std::string requestMethod;
        std::string requestRessource;
        std::string httpVersion;
        std::map<std::string, std::string> headers;
        std::string body;

        int checkAllowedChars(std::string value);
        void parseRequest(std::string buffer, std::string delim);
        void parseRequestLine(std::string buffer);
        void parseHeaders(std::string buffer);
        std::string& trimSpaces(std::string& val);
    public:
        Request(std::string buffer);
        ~Request();
        std::string getRequestMethod() const;
        std::string getRequestRessource() const;
        std::string getHttpVersion() const;
        std::string getHeader(std::string key) const;
        std::string getBody() const;
        Response handleRequest();
        void printHeaders();
};