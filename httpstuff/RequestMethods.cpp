#include "RequestMethods.hpp"
#include "Response.hpp"
#include <fstream>
#include <sstream>

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

Response RequestMethod::GET(Request& request) {
    // host in request
    std::string requestedRessource = request.getRequestRessource();
    if (!request.getHeader("host").empty()) {

    } else if (requestedRessource.compare("/") == 0) {
        Response response = buildResponseWithFile("index.html");
        return (response);
    } else if (requestedRessource[requestedRessource.size() - 1] == '/') {

    } else {
        Response response = buildResponseWithFile(requestedRessource.substr(1));
        return (response);
    }
    Response response = buildResponseWithFile("");
    return (response);
}