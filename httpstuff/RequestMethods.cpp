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
        if (entry->d_name[0] != '.')
            ss << "<li>" << entry->d_name << "</li>";
    }

    ss << "</ul></body></html>";
    closedir(dir);
    return (ss.str());
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
                        ss << generateHTML(path.c_str());
                        response.setContentType(".html");
                        response.setContentLength(ss.str().size());
                        response.setResponseBody(ss.str());
                        response.buildResponse(OK);
                    } else {
                        response.buildResponse(FORBIDDEN);
                    }
                } else {
                    ss << file.rdbuf();
                    response.setContentType(".html");
                    response.setContentLength(ss.str().size());
                    response.setResponseBody(ss.str());
                    response.buildResponse(OK);
                }
            } else {
                std::ifstream file(path + "index.html");
                ss << file.rdbuf();
                response.setContentType(".html");
                response.setContentLength(ss.str().size());
                response.setResponseBody(ss.str());
                response.buildResponse(OK);
            }
        }
        else {
            std::ifstream file(path);
            if (!file.is_open()) {
                response.buildResponse(NOT_FOUND);
            } else {
                ss << file.rdbuf();
                response.setContentType(path);
                response.setContentLength(ss.str().size());
                response.setResponseBody(ss.str());
                response.buildResponse(OK);
                // ss << http << 404 << NOT FOUND << "\r\n" content-length: << 400 << content-type: << txt << "\rn" << body
                // response.setresponsentity(ss.str());
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
        return (response);
    } else if (requestedRessource[requestedRessource.size() - 1] == '/') {
        // if request wants a directory
        response = buildResponseWithFile(request, config, request.getPath(), OK);
    } else {
        // specific ressource is requested instead of default
        if (requestedRessource.substr(1).find('/') != std::string::npos) {
            // if request has a directory then trim the request to contain the file only
            size_t pos = requestedRessource.find('/', 1);
            response = buildResponseWithFile(request, config, request.getPath() + requestedRessource.substr(pos + 1), OK);
        } else {
            // request doesn't have a dir
            response = buildResponseWithFile(request, config, request.getPath() + requestedRessource.substr(1), OK);
        }
    }
    return (response);
}