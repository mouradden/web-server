#include "RequestMethods.hpp"
#include "Response.hpp"
#include <fstream>
#include <sstream>
#include <vector>

// void setChunkedResponse(Response &response, std::string filename, unsigned int code) {
//     std::ostringstream ss;
//     if (response.getState() == 1) {
//         response.setStatus(code);
//         ss << "HTTP/1.1 " << code << " " << response.getStatus() << "\r\n";
//         ss << "Content-Type: " << response.getMimeType(filename.substr(filename.find_last_of('.') + 1)) << "\r\n";
//         ss << "Transfer-Encoding: chunked\r\n";
//         // ss << "5" << "hello\r\n"
//         response.setResponseEntity(ss.str());
//         response.setState(2);
//     } else if (response.getState() == 2) {
//         response.setResponseEntity("");
//         std::ifstream file(filename);
//         if (!file.is_open()) {
//             response.setState(-1);
//             ss << "HTTP/1.1" << NOT_FOUND << "Not Found\r\n";
//             response.setResponseEntity(ss.str());
//             return ;
//         }
//         if (response.getFileOffset() != 0) {
//             file.seekg(response.getFileOffset());
//         }
//         int buffersize = 100;
//         char buffer[buffersize - 1];
//         file.read(buffer, buffersize);
//         std::streamsize bytesread = file.gcount();
//         buffer[bytesread] = '\0';
//         response.setFileOffset(response.getFileOffset() + bytesread);
//         ss << std::hex << bytesread + 2 << "\r\n" << buffer << "\r\n";
//         if (file.eof()) {
//             ss << "0\r\n\r\n";
//             response.setState(-1);
//         }
//         response.setResponseEntity(ss.str());
//         file.close();
//     }
// }

void buildResponseWithFile(Response& response, std::string filename, unsigned int code) {
    std::ostringstream ss;
    std::ifstream file(filename);
    if (!(code >= 200 && code <= 208)) {
        response.buildResponse(code);
        return ;
    } else {
        if (file.fail()) {
                response.buildResponse(NOT_FOUND);
                return ;
            } else {
                ss << file.rdbuf();
                // if (ss.str().size() > 100000) {
                //     if (response.getState() == 0) {
                //         response.setState(1);
                //     }
                //     setChunkedResponse(response, filename, OK);
                // } else {
                    response.setContentType(filename);
                    response.setContentLength(ss.str().size());
                    response.setResponseBody(ss.str());
                    response.buildResponse(OK);
                // }
                return ;
            }
    }
}


Response RequestMethod::GET(Request& request, DataConfig config) {
    std::string requestedRessource = request.getRequestRessource();
    Response response;
    if (requestedRessource.compare("/") == 0) {
        // request is empty, send index of root
        buildResponseWithFile(response, request.getPath() + config.getIndex(), OK);
        return (response);
    } else if (requestedRessource[requestedRessource.size() - 1] == '/') {
        // if request wants a directory
        std::vector<Location>::iterator locationData = config.getSpecificLocation(request.getLocation());
        if (locationData != config.getLocation().end()) {
            if (locationData->methods.get == 0) {
                // if the location is found but the http method isn't allowed
                buildResponseWithFile(response, "", METHOD_NOT_ALLOWED);
            } else {
                // if the location is found and the http method is allowed
                buildResponseWithFile(response, request.getPath() + locationData->index, OK);
            }
        } else {
            // directory isn't present in the locations, try to send a response with the index of the directory
            buildResponseWithFile(response, request.getPath() + "index.html", OK);
        }
    } else {
        // specific ressource is requested instead of default
        if (requestedRessource.substr(1).find('/') != std::string::npos) {
            // if request has a directory then trim the request to contain the file only
            size_t pos = requestedRessource.find('/', 1);
            buildResponseWithFile(response, request.getPath() + requestedRessource.substr(pos + 1), OK);
        } else {
            // request doesn't have a dir
            buildResponseWithFile(response, request.getPath() + requestedRessource.substr(1), OK);
        }
    }
    return (response);
}