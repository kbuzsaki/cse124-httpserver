#ifndef HANDLER_H
#define HANDLER_H

#include "http.h"

class HttpHandler {
public:
    virtual ~HttpHandler() {};

    virtual HttpResponse handle_request(const HttpRequest&) = 0;
};

#endif //HANDLER_H
