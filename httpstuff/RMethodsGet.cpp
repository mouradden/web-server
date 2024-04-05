#include "Request.hpp"
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

void buildResponseWithCgi(Response& response, DataConfig& config, Request& request, std::string path) {
    CgiOutput  data;

    data = Cgi::CallCgi(path, request, "/", config);
    // std::cout <<"hiii    " << data.getBody() << "hiiiiiiiiii am her \n\n\n\n\n\n\n\n";
    if(data.getCgiError() == "error")
        response.buildResponse(config, request.getLocation(), INTERNAL_SERVER_ERROR);
    else if(data.getCgiError() == "time out")
        response.buildResponse(GATEWAY_TIMEOUT);
    else if(data.getLocation().empty())
    {
        response.setContentType(path);
        response.setContentLength(data.getBody().size());
        response.setResponseBody(data.getBody());
        response.buildResponse(OK);
    }
    else
    {
        response.setHeader("Location:", data.getLocation());
        response.buildResponse(TEMPORARY_REDIRECT);
    }
}

void fillResponse(Response &response, std::ostringstream& ss, std::string filetype) {
    response.setContentType(filetype);
    response.setContentLength(ss.str().size());
    response.setResponseBody(ss.str());
    // response.setHeader("Transfer-Encoding:", "chunked");
    response.buildResponse(OK);
}

void handleFolder(Response &response, std::vector<Location>::iterator &it, DataConfig &config, Request &request) {
    std::ostringstream ss;
    std::string path = request.getPath();
    if (it != config.getLocation().end()) {
        std::string indexFile = it->index.empty() ? config.getIndex() : it->index;
        std::ifstream file(path + indexFile);
        
        // cgi part
        size_t pos = indexFile.find_last_of(".");
        std::string extension = "";
        if (pos != std::string::npos) {
            extension = indexFile.substr(pos);
        }
        if (extension == ".php") {
            buildResponseWithCgi(response, config, request, path + indexFile);
            return ;
        }
        // cgi end

        if (!file.is_open()) {
            if (it->autoIndex) {
                ss << generateHTML(path.c_str());
                fillResponse(response, ss, ".html");
            } else {
                response.buildResponse(config, request.getLocation(), FORBIDDEN);
            }
        } else {
            ss << file.rdbuf();
            fillResponse(response, ss, indexFile);
        }
    } else if (it == config.getLocation().end()) {
        std::ifstream file(path + config.getIndex());
        if (!file.is_open()) {
            if (config.getAutoIndex()) {
                ss << generateHTML(path.c_str());
                fillResponse(response, ss, ".html");
            } else {
                response.buildResponse(config, request.getLocation(), FORBIDDEN);
            }
        } else {
            ss << file.rdbuf();
            fillResponse(response, ss, config.getIndex());
        }
    }
}

void handleFile(Response &response, std::vector<Location>::iterator &it, DataConfig &config, Request &request) {
    std::ostringstream ss;
    std::cout << it->_return.path;
    std::string path = request.getPath();
    
    size_t pos = path.find_last_of(".");
    std::string extension = "";
    if (pos != std::string::npos) {
        extension = path.substr(pos);
    }
    if (extension == ".php") {
        if (it->cgiExtension.empty()) {
            buildResponseWithCgi(response, config, request, path);
            return ;
        } else {
            response.buildResponse(config, request.getLocation(), FORBIDDEN);
            return ;
        }
    } 
    std::ifstream file(path);
    if (!file.is_open()) {
        response.buildResponse(config, request.getLocation(), NOT_FOUND);
    } else {
        ss << file.rdbuf();
        fillResponse(response, ss, path);
    }
}

Response buildResponseWithFile(DataConfig config, Request &request) {
    std::ostringstream ss;
    Response response;

    std::string path = request.getPath();
    std::string location = request.getLocation();
    std::vector<Location>::iterator locationData = config.getSpecificLocation(location);
    if (path.back() == '/') {
        handleFolder(response, locationData, config, request);
    } else {
        handleFile(response, locationData, config, request);
    } 
    return (response);
}


Response RequestMethod::GET(Request& request, DataConfig config) {
    Response response;
    std::string requestedRessource = request.getRequestRessource();
    if (requestedRessource[requestedRessource.size() - 1] == '/') {
        // if request wants a directory
        response = buildResponseWithFile(config, request);
    } else {
        // specific ressource is requested instead of default
        response = buildResponseWithFile(config, request);
    }
    return (response);
}
