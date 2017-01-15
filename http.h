#ifndef HTTP_H
#define HTTP_H

#include <vector>
#include "connection.h"

struct HttpHeader {
    std::string key;
    std::string value;
};

struct HttpRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::vector<HttpHeader> headers;
};

struct HttpResponse {
    std::string version;
    int status;
    std::vector<HttpHeader> headers;
    std::string body;
};

class HttpConnection {
    BufferedConnection conn;
    
public:
    HttpConnection(BufferedConnection&& conn);
    HttpRequest read_request();
    void write_response(HttpResponse);
};

#endif //HTTP_H
