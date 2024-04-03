#pragma once

#include "../parse/DataConfig.hpp"
#include "../Server.hpp"
#include "../cgi/Cgi.hpp"
#include "../cgi/CgiOutput.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <cstdio>
#include <dirent.h>


class Request;
class Response;
class RequestMethod {
    public:
        static Response GET(Request& request, DataConfig config);
         static Response POST(Request& request, DataConfig config);
        static Response DELETE(Request& request, DataConfig config);
};