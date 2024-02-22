#include "Response.hpp"
#include "Server.hpp"
#include <sstream>

std::string getContentType(std::string fileExtension) {
    std::map<std::string, std::string> contentType;
    contentType.insert(std::make_pair("txt", "text/plain"));
    contentType.insert(std::make_pair("html", "text/html"));
    contentType.insert(std::make_pair("htm", "text/html"));
    contentType.insert(std::make_pair("css", "text/css"));
    contentType.insert(std::make_pair("js", "text/javascript"));
    contentType.insert(std::make_pair("json", "application/json"));
    contentType.insert(std::make_pair("jpg", "image/jpeg"));
    contentType.insert(std::make_pair("jpeg", "image/jpeg"));
    contentType.insert(std::make_pair("png", "image/png"));
    contentType.insert(std::make_pair("gif", "image/gif"));
    contentType.insert(std::make_pair("svg", "image/svg+xml"));
    std::map<std::string, std::string>::iterator it = contentType.find(fileExtension);
    if (it != contentType.end()) {
        return (it->second);
    } else {
        return ("application/octet-stream");
    }
}

Response::Response(std::string httpVersion, unsigned int code) : httpVersion(httpVersion), code(code) {
    setStatus(code);
    contentType = "";
    contentLength = 0;
    body = "";
    responseEntity = "";
};

Response::Response(const Response& ref) {
    httpVersion = ref.httpVersion;
    code = ref.code;
    status = ref.status;
    contentType = ref.contentType;
    contentLength = ref.contentLength;
    body = ref.body;
    responseEntity = ref.responseEntity;
};


Response& Response::operator=(const Response &ref) {
    responseEntity = ref.responseEntity;
    httpVersion = ref.httpVersion;
    code = ref.code;
    status = ref.status;
    contentType = ref.contentType;
    contentLength = ref.contentLength;
    body = ref.body;
    return (*this);
}

void Response::buildResponse() {
    std::ostringstream ss;
    ss << httpVersion << " " << code << " " << status << "\r\n" 
    << "Content-Type: " << contentType << "\r\n" 
    << "Content-Length: " << contentLength << "\r\n\r\n" 
    << body;
    responseEntity = ss.str();
}

void Response::sendResponse(int socket) {
    send(socket, responseEntity.c_str(), responseEntity.size(), 0);
}

std::string Response::getResponseEntity() {
    return (responseEntity);
}

// ************ SETTERS ************

void Response::setContentType(std::string fileExtension) {
    contentType = fileExtension;
}

void Response::setContentLength(unsigned int length) {
    contentLength = length;
}

void Response::setResponseBody(std::string content) {
    body = content;
}

void Response::setStatus(unsigned int code) {
    switch(code) {
        case 200:
             status = "OK";
             break ;
        // 4XX status codes
        case BAD_REQUEST:
            status =  "Bad Request";
            break ;
        case FORBIDDEN:
            status =  "Forbidden";
            break ;
        case NOT_FOUND:
            status =  "Not Found";
            break ;
        case METHOD_NOT_ALLOWED:
            status =  "Method Not Allowed";
            break ;
        case NOT_ACCEPTABLE:
            status =  "Not Acceptable";
            break ;
        case LENGTH_REQUIRED:
            status =  "Length Required";
            break ;
        case ENTITY_LENGTH_EXCEEDED:
            status =  "Payload Too Large";
            break ;
        case REQUEST_URI_EXCEEDED:
            status =  "URI Too Long";
            break ;
        default:
            status = "";
            break ;
    }
}