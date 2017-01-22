#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include "http.h"
#include "listener.h"

class HttpListener {
    std::shared_ptr<Listener> listener;

public:
    HttpListener(std::shared_ptr<Listener>);
    HttpListener(HttpListener&&);

    void listen();
    HttpConnection accept();
};


class HttpRequestHandler {
public:
    virtual ~HttpRequestHandler() {};

    virtual HttpResponse handle_request(const HttpRequest&) = 0;
};


class HttpServer {
    HttpListener listener;
    std::shared_ptr<HttpRequestHandler> handler;

public:
    HttpServer(HttpListener&&, std::shared_ptr<HttpRequestHandler>);

    void serve();
};

#endif //SERVER_H
