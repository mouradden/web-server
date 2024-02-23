#pragma once

#include "../Server.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Request;

class RequestMethod {
    public:
        static Response GET(Request& request);
        static Response POST(Request& request);
        static Response DELETE(Request& request);
};