#include "Response.hpp"

Response::Response(std::string res, std::string httpVersion, unsigned int code, std::string status) : res(res), httpVersion(httpVersion), code(code), status(status) {
};

Response::Response(const Response& ref) : res(ref.res), httpVersion(ref.httpVersion), code(ref.code), status(ref.status) {
};

Response& Response::operator=(const Response &ref) {
    res = ref.res;
    httpVersion = ref.httpVersion;
    code = ref.code;
    status = ref.status;
    return (*this);
}

void Response::buildResponse(std::string fileContent, std::string fileType) {
    std::ostringstream ss;
    ss << httpVersion << " " << code << " " << status << "\r\n" 
    << "Content-Type: " << fileType << "\r\n" << "Content-Length: " << fileContent.size() << "\r\n\r\n" 
    << fileContent;
    res = ss.str();
}

void Response::sendResponse(int socket) {
    send(socket, res.c_str(), res.size(), 0);
}