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

    ss << "<html><head><title>Directory Listing</title><style>h1 {text-align:center;}</style></head><body><h1>Directory Listing</h1><ul><br>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            ss << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
        }
    }

    ss << "</ul></body></html>";
    closedir(dir);
    return ss.str();
}

void fillResponse(Response &response, std::ostringstream& ss, std::string filetype) {
    response.setContentType(filetype);
    response.setContentLength(ss.str().size());
    response.setResponseBody(ss.str());
    response.buildResponse(OK);
}

void handleFolder(Response &response, std::vector<Location>::iterator &it, DataConfig &config, std::string path) {
    std::ostringstream ss;
    if (it != config.getLocation().end()) {
        std::string filename = it->index.empty() ? config.getIndex() : it->index;
        std::ifstream file(path + filename);
        if (!file.is_open()) {
            if (it->autoIndex) {
                ss << generateHTML(path.c_str());
                fillResponse(response, ss, ".html");
            } else {
                response.buildResponse(FORBIDDEN);
            }
        } else {
            ss << file.rdbuf();
            fillResponse(response, ss, it->index);
        }
    } else if (it == config.getLocation().end()) {
        std::ifstream file(path + config.getIndex());
        if (!file.is_open()) {
            if (config.getAutoIndex()) {
                ss << generateHTML(path.c_str());
                fillResponse(response, ss, ".html");
            } else {
                response.buildResponse(FORBIDDEN);
            }
        } else {
            ss << file.rdbuf();
            fillResponse(response, ss, config.getIndex());
        }
    }
}

void handleFile(Response &response, std::string path) {
    std::ostringstream ss;
    std::ifstream file(path);
    if (!file.is_open()) {
        response.buildResponse(NOT_FOUND);
    } else {
        ss << file.rdbuf();
        fillResponse(response, ss, path);
    }
}

Response buildResponseWithFile(DataConfig config, std::string path, std::string location) {
    std::ostringstream ss;
    Response response;
   
    if (path.back() == '/') {
        std::vector<Location>::iterator locationData = config.getSpecificLocation(location);
        handleFolder(response, locationData, config, path);
    } else {
        handleFile(response, path);
    } 
    return (response);
}


Response RequestMethod::GET(Request& request, DataConfig config) {
    Response response;
    std::string requestedRessource = request.getRequestRessource();
    if (requestedRessource[requestedRessource.size() - 1] == '/') {
        // if request wants a directory
        response = buildResponseWithFile(config, request.getPath(), request.getLocation());
    } else {
        // specific ressource is requested instead of default
        response = buildResponseWithFile(config, request.getPath(), request.getLocation());
    }
    return (response);
}
