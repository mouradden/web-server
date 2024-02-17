#include "Request.hpp"
#include "Response.hpp"
#include <cctype>
#include <cstddef>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

//  ******** PARSING METHODS ********

std::string& Request::trimSpaces(std::string& val) {
    std::string whiteSpaces = "\t ";
    val.erase(val.find_last_not_of(whiteSpaces) + 1);
    val.erase(0, val.find_first_not_of(whiteSpaces));
    return (val);
}

void Request::parseRequestLine(std::string buffer) {
    std::stringstream requestLine(buffer);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(requestLine, token, ' ')) {
        tokens.push_back(token);
    }
    this->requestMethod = trimSpaces(tokens[0]);
    this->requestRessource = trimSpaces(tokens[1]);
    this->httpVersion = trimSpaces(tokens[2]);
}

void Request::parseHeaders(std::string buffer) {
    std::stringstream requestLine(buffer);
    std::string key;
    std::string value;

    std::getline(requestLine, key, ':');
    std::getline(requestLine, value, ':');
    this->headers[trimSpaces(key)] = trimSpaces(value);
}

void Request::parseRequest(std::string buffer, std::string delim) {
    std::vector<std::string> values;
    std::string line;
    size_t pos = 0;

    while ((pos = buffer.find(delim))!= std::string::npos) {
        if (buffer.compare(0, 4, delim, 0, 4) == 0) {
            values.push_back(delim);
        }
        line = buffer.substr(0, pos);
        values.push_back(line);
        buffer.erase(0, pos + delim.size());
    }
    if (!buffer.empty()) {
        body = buffer;
    }
    for (size_t i = 0; i < values.size() && values[i].compare(delim) != 0; i++) {
        if (i == 0) {
            parseRequestLine(values[i]);
        } else {
            parseHeaders(values[i]);
        }
    }
}

int Request::validRequest() {
    if (headers.find("Transfer-Encoding") != headers.end()) {
        std::cout << "transfer encoding found\n";
        if (headers["Transfer-Encoding"] != "chunked") {
            std::cout << "transfer not implemented\n";
            return (501);
        }
    }
    if (requestMethod == "POST" && (headers.find("Transfer-Encoding") == headers.end() || headers.find("Content-Length") == headers.end())) {
        std::cout << "POST bad request\n";
        return (400);
    }
    if (checkAllowedChars(requestRessource) == 400) {
        std::cout << "request uri : Bad request\n";
        return (400);
    }
    if (requestRessource.size() > 2048)
    {
        std::cout << "request uri : exceeded 2048\n";
        return (414);
    }
    // if client request body is larger than maximum body allowed in config file (change 8000 value to config file value
    if (requestEntity.size() > 8000) {
        std::cout << "request entity too large\n";
        return (413);
    }
    return (0);
}

int Request::checkAllowedChars(std::string value) {
    std::string allowedChars = "-._~:/?#[]@!$&&\'()*+;=%}";
    for (size_t i = 0; i < value.size(); i++) {
        if (!std::isalpha(value[i]) && !std::isdigit(value[i]) && allowedChars.find(value[i]) == std::string::npos) {
            // change the hardcoded value into a macro
            return (400);
        }
    }
    return (0);
}

//  ******** CONSTRUCTORS ********

Request::Request(std::string buffer) {
    requestEntity = buffer;
    parseRequest(buffer, "\r\n");
}

Request::~Request() {

}

//  ******** GETTERS ********

std::string Request::getRequestMethod() const {
    return (this->requestMethod);
}

std::string Request::getRequestRessource() const {
    return (this->requestRessource);
}

std::string Request::getHttpVersion() const {
    return (this->httpVersion);
}

std::string Request::getHeader(std::string key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return (it->second);
    else
        return "";
}

std::string Request::getBody() const {
    return (this->body);
}

void    Request::printHeaders() {
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++) {
        std::cout << it->first << " " << it->second << std::endl;
    }
}

//  ******** HANDLER ********

Response Request::handleRequest() {
    // check request syntax (encoding - uri length - allowed chars - post but no message content information - request length)
    // check if uri exists
    // check if operation is allowed
    int code = validRequest();
    if (code != 0) {
        std::cout << code << std::endl;
    }
    if (requestMethod == "GET") {
        if (requestRessource == "/") {
            std::ostringstream ss;
            std::ifstream file("index.html");
            if (!file) {
                Response response("", "HTTP/1.1", 404,"File not found");
                response.buildResponse("", "text/html");
                return response;
            }
            ss << file.rdbuf();
            std::string fileContent = ss.str();
            if (file.fail()) {
                Response response("", "HTTP/1.1", 500,"Server error");
                response.buildResponse("", "text/html");
                return response;
            } else {
                Response response("", "HTTP/1.1", 200,"ok");
                response.buildResponse(fileContent, "text/html");
                return response;
            }
        }
    }
    Response response("", "HTTP/1.1", 500,"server error");
    response.buildResponse("", "text/html");
    return response;
}