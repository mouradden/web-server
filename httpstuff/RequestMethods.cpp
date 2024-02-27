#include "RequestMethods.hpp"
#include "Response.hpp"
#include <fstream>
#include <sstream>
#include <vector>

Response buildResponseWithFile(std::string filename) {
    std::ostringstream ss;
    std::ifstream file(filename);
    if (file.fail()) {
        Response response("HTTP/1.1", NOT_FOUND);
        response.buildResponse();
        return (response);
    } else {
        Response response("HTTP/1.1", OK);
        ss << file.rdbuf();
        response.setContentType("index.html");
        response.setContentLength(ss.str().size());
        response.setResponseBody(ss.str());
        response.buildResponse();
        return response;
    }
}

int checkIfLocationExists(std::vector<Location> locations, std::string requestedRessource) {
    for (size_t i = 0; i < locations.size(); i++) {
        if (locations[i].location.compare(requestedRessource) == 0)
            return (i);
    }
    return (-1);
}

Response RequestMethod::GET(Request& request, DataConfig config) {
    std::string requestedRessource = request.getRequestRessource();
    if (requestedRessource.compare("/") == 0) {
        Response response = buildResponseWithFile(config.getIndex());
        return (response);
    } else if (requestedRessource[requestedRessource.size() - 1] == '/') {
        std::vector<Location> locations = config.getLocation();
        int index = checkIfLocationExists(locations, requestedRessource);
        if (index != -1) {
            if (locations[index].methods.get == 0) {
                Response response("HTTP/1.1", 405);
                buildResponseWithFile("");
                return (response);
            }
            Response response = buildResponseWithFile(locations[index].index);
            return (response);
        } else {
            Response response = buildResponseWithFile("");
            return (response);
        }
    } else {
        Response response = buildResponseWithFile(requestedRessource.substr(1));
        return (response);
    }
    Response response = buildResponseWithFile("");
    return (response);
}