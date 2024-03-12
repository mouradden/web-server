#include "RequestMethods.hpp"
#include "Response.hpp"
#include <fstream>
#include <sstream>
#include <vector>

std::string generateHTML(const char* path) {
    std::ostringstream ss;
    DIR *dir = opendir(path);
    if (!dir) {
        return "";
    }

    ss << "<html><head><title>Directory Listing</title><style>h1 {text-align:center;}</style></head><body><h1>Directory Listing</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            ss << "<li><a href=\"" << std::string(path) + entry->d_name << "\">" << entry->d_name << "</a></li>";
        }
    }

    ss << "</ul></body></html>";
    closedir(dir);
    return ss.str();
}

Response buildResponseWithFile(Request request, DataConfig config, std::string path, unsigned int code) {
    std::ostringstream ss;
    Response response;
    if (!(code >= 200 && code <= 208)) {
        response.buildResponse(code);
    } else {
        if (path.back() == '/') {
            std::vector<Location>::iterator locationData = config.getSpecificLocation(request.getLocation());
            if (locationData != config.getLocation().end()) {
                std::ifstream file(path + locationData->index);
                if (!file.is_open()) {
                    if (locationData->autoIndex) {
                        std::cout << "entered check 1\n";
                        ss << generateHTML(path.c_str());
                        response.setContentType(".html");
                        response.setContentLength(ss.str().size());
                        response.setResponseBody(ss.str());
                        response.buildResponse(OK);
                    } else {
                        response.buildResponse(FORBIDDEN);
                    }
                } else {
                    std::cout << "entered check 2\n";
                    std::ifstream file(path + locationData->index);
                    ss << file.rdbuf();
                    response.setContentType(locationData->index);
                    response.setContentLength(ss.str().size());
                    response.setResponseBody(ss.str());
                    response.buildResponse(OK);
                }
            } else {
                std::cout << "entered check 3\n";
                std::ifstream file(path + config.getIndex());
                ss << file.rdbuf();
                response.setContentType(config.getIndex());
                response.setContentLength(ss.str().size());
                response.setResponseBody(ss.str());
                response.buildResponse(OK);
            }
        }
        else {
            std::cout << "entered check 4\n";
            std::ifstream file(path);
            if (!file.is_open()) {
                response.buildResponse(NOT_FOUND);
            } else {
                ss << file.rdbuf();
                response.setContentType(path);
                response.setContentLength(ss.str().size());
                response.setResponseBody(ss.str());
                response.buildResponse(OK);
            }
        } 
    }
    return (response);
}


Response RequestMethod::GET(Request& request, DataConfig config) {
    std::string requestedRessource = request.getRequestRessource();
    Response response;
    if (requestedRessource.compare("/") == 0) {
        // request is empty, send index of root
        response = buildResponseWithFile(request, config, request.getPath() + config.getIndex(), OK);
    } else if (requestedRessource[requestedRessource.size() - 1] == '/') {
        // if request wants a directory
        std::cout << "entered directory test\n";
        response = buildResponseWithFile(request, config, request.getPath(), OK);
    } else {
        // specific ressource is requested instead of default
        response = buildResponseWithFile(request, config, request.getPath(), OK);
    }
    return (response);
}