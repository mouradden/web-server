#pragma once

#include "../parse/DataConfig.hpp"
#include "../Server.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Request;
class Response;
class RequestMethod {
    public:
        static Response GET(Request& request, DataConfig config);
        static Response POST(Request& request);
        static Response DELETE(Request& request);
};