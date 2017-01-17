#ifndef SERVER_H
#define SERVER_H

#include "connection.h"
#include "http.h"
#include "handler.h"

class HttpListener {
    SocketListener listener;

public:
    HttpListener(SocketListener&&);
    HttpListener(HttpListener&&);

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
