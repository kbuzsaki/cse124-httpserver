#ifndef SERVER_H
#define SERVER_H

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


class HttpHandler {
public:
    virtual ~HttpHandler() {};

    virtual HttpResponse handle_request(const HttpRequest&) = 0;
};


class HttpServer {
    HttpListener listener;
    HttpHandler* handler;

public:
    HttpServer(HttpListener&&, HttpHandler*);
    ~HttpServer();

    void serve();
};

#endif //SERVER_H
