#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include "http.h"
#include "listener.h"


class HttpConnection {
    BufferedConnection conn;

    HttpFrame read_frame();
    void write_frame(HttpFrame frame);

public:
    HttpConnection(std::shared_ptr<Connection> conn);
    HttpConnection(HttpConnection&&);

    HttpRequest read_request();
    void write_response(HttpResponse);
};


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


class HttpConnectionHandler {
public:
    virtual ~HttpConnectionHandler() {};

    virtual void handle_connection(HttpConnection&&) = 0;
};


class HttpServer {
    HttpListener listener;
    std::shared_ptr<HttpConnectionHandler> handler;

public:
    HttpServer(HttpListener&&, std::shared_ptr<HttpConnectionHandler>);

    void serve();
};

#endif //SERVER_H
