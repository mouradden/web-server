#include "Request.hpp"
#include "RequestMethods.hpp"
#include "Response.hpp"
#include <string>
#include <vector>
#include <sys/stat.h>

//  ******** PARSING METHODS ********


//  ******** CONSTRUCTORS ********

Request::Request(std::string buffer) {
    requestEntity = buffer;
    parseRequest(buffer, "\r\n");
    path = "";
    location = "";
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

std::string Request::getPath() const {
    return (this->path);
}

std::string Request::getLocation() const {
    return (this->location);
}

std::string& Request::trimSpaces(std::string& val) {
    std::string whiteSpaces = "\t ";
    val.erase(val.find_last_not_of(whiteSpaces) + 1);
    val.erase(0, val.find_first_not_of(whiteSpaces));
    if (val[val.size() - 1] == '?')
        val = val.substr(0, val.size() - 1);
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

int checkIfFolder(DataConfig config, std::string requestedRessource) {
    struct stat fileInfo;
    std::vector<Location>::iterator it = config.getSpecificLocation(requestedRessource + "/");
    if (it != config.getLocation().end()) {
        return (PERMANENTLY_MOVED);
    } else if (stat((config.getRoot() + requestedRessource.substr(1)).c_str(), &fileInfo) != 0) {
        return (NOT_FOUND);
    } else {
        if (S_ISDIR(fileInfo.st_mode) && requestedRessource[requestedRessource.size() - 1] != '/') {
            return (PERMANENTLY_MOVED);
        }
    }
    return (0);
}

int Request::validRequest(DataConfig config) {
    config.getRoot();
    if (headers.find("Transfer-Encoding") != headers.end()) {
        std::cout << "transfer encoding found\n";
        if (headers["Transfer-Encoding"] != "chunked") {
            return (501);
        }
    }
    if (requestMethod == "POST" && (headers.find("Content-Length") == headers.end()))
        return (BAD_REQUEST);
    if (checkAllowedChars(requestRessource) == 400) {
        std::cout << "request uri : Bad request\n";
        return (BAD_REQUEST);
    }
    if (requestRessource.size() > 2048)
    {
        std::cout << "request uri : exceeded 2048\n";
        return (REQUEST_URI_EXCEEDED);
    }
    // if client request body is larger than maximum body allowed in config file (change 8000 value to config file value
    if (requestEntity.size() > 8000) {
        std::cout << "request entity too large\n";
        return (ENTITY_LENGTH_EXCEEDED);
    }
    // if (requestRessource.back() != '/') {
    //     int code = checkIfFolder(config, requestRessource);
    //     if (code != 0) {
    //         return (code);
    //     }
    // }
    if (requestMethod.compare("GET") != 0 && requestMethod.compare("POST") != 0 && requestMethod.compare("DELETE") != 0) {
        return (NOT_IMPLEMENTED);
    }
    return (0);
}

void Request::buildPath(DataConfig config) {
    if (requestRessource.substr(1).find('/') != std::string::npos) {
        // if a directory is requested search if it's exists in a location
        std::string requestedLocation = requestRessource.substr(0, requestRessource.find_last_of('/') + 1);
        std::vector<Location>::iterator locationData = config.getSpecificLocation(requestedLocation);
        if (locationData != config.getLocation().end()) {
            this->location = locationData->location;
            if (locationData->root.empty() && locationData->alias.empty()) {
                path = config.getRoot() + requestedLocation.substr(1);
            } else if (locationData->root.empty()) {
                path = config.getRoot() + locationData->alias.substr(1);
            } else {
                path = locationData->root;
            }
        } else {
            path = config.getRoot() + requestedLocation.substr(1);
        }
    } else {
        path = config.getRoot();
        location = "/";
    }
}

//  ******** HANDLER ********

int Request::methodAllowed(DataConfig config) {
    std::vector<Location>::iterator locationData = config.getSpecificLocation(location);
    if (locationData != config.getLocation().end()) {
        if (requestMethod.compare("GET") == 0) {
            if (locationData->methods.get == 0)
                return (0);
        } else if (requestMethod.compare("POST") == 0) {
            if (locationData->methods.post == 0)
                return (0);
        } else if (requestMethod.compare("DELETE") == 0) {
            if (locationData->methods._delete == 0)
                return (0);
        }
    } else if (requestRessource == "/") {
        if (requestMethod.compare("GET") == 0)
            return (0);
    }
    return (1);
}

void        Request::parseHostPort()
{
    std::string hostKey = "Host: ";
    std::string line;
    size_t hostStart = this->requestEntity.find(hostKey);
    if (hostStart < this->requestEntity.length()) {
        size_t lineEnd = this->requestEntity.find("\n", hostStart);
        if (lineEnd < this->requestEntity.length())
        {
            line = this->requestEntity.substr(hostStart, lineEnd - hostStart);
            lineEnd = 0;
            hostStart = 0;
            hostStart = line.find(" ");
            lineEnd = line.find(":", hostStart);
            this->host = line.substr(hostStart+1, (lineEnd - hostStart) - 1);
            lineEnd = 0;
            hostStart = 0;
            int doublePoint = 0;
            hostStart = line.find(":");
            doublePoint = line.find(":", hostStart+1);
            this->port = line.substr(doublePoint+1, (line.length() - doublePoint));
        }
         else 
            line = "";
    }
}

void    Request::checkWichServer()
{
    parseHostPort();
    
}
Response Request::runHttpMethod(DataConfig config) {
    Response response;
    if (requestMethod.compare("GET") == 0) {
        response = RequestMethod::GET(*this, config);
    } 
    else if (requestMethod.compare("POST") == 0) {
        checkWichServer();
        // std::cout <<"host " << this->host << "\n";

        std::cout << this->requestEntity << "\n";
        std::cout << "getLocation  " << config.getLocation()[1].alias << "\n";
        // std::cout << this->httpVersion << "\n";
        
        // response = RequestMethod::POST(*this, config);
    }
    //  else if (requestMethod.compare("DELETE") == 0) {
    //     response = RequestMethod::DELETE(*this, config);
    // }
    return (response);
}

Response Request::handleRequest(DataConfig config) {
    int errorCode = validRequest(config);
    if (errorCode != 0) {
        Response response;
        if (errorCode == PERMANENTLY_MOVED) {
            response.setHeader("Location:", requestRessource + "/");
        }
        response.buildResponse(errorCode);
        return (response);
    }
    buildPath(config);
    if (!methodAllowed(config)) {
        Response response;
        response.buildResponse(METHOD_NOT_ALLOWED);
        return (response);
    }
    Response response = runHttpMethod(config);
    return response;
}