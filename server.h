#ifndef SERVER_H
#define SERVER_H

#include "http.h"
#include "handler.h"
#include "listener.h"

class HttpListener {
    Listener* listener;

public:
    HttpListener(Listener*);
    HttpListener(HttpListener&&);
    ~HttpListener();

    void listen();
    HttpConnection accept();
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
