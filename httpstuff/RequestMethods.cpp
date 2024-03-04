#include "RequestMethods.hpp"
#include "Response.hpp"
#include <fstream>
#include <sstream>
#include <vector>

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
                // if (ss.str().size() > 2000) {
                //     // std::cout << "transfer needs to be chunked\n";
                //     response.setContentType(filename);
                //     size_t nread = 1;
                //     std::string buffer;
                //     while (nread != 0) {
                //         nread = 
                //     }
                // }
                response.setContentType(filename);
                response.setContentLength(ss.str().size());
                response.setResponseBody(ss.str());
                response.buildResponse(OK);
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