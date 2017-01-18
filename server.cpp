#include "server.h"

HttpListener::HttpListener(Listener* listener) : listener(listener) {}

HttpListener::HttpListener(HttpListener&& listener) : listener(listener.listener) {
    listener.listener = NULL;
}

HttpListener::~HttpListener() {
    delete listener;
}

void HttpListener::listen() {
    listener->listen();
}

HttpConnection HttpListener::accept() {
    return HttpConnection(listener->accept());
}


HttpServer::HttpServer(HttpListener&& listener, HttpHandler* handler) : listener(std::move(listener)), handler(handler) {}

HttpServer::~HttpServer() {
    delete handler;
}

void HttpServer::serve() {
    listener.listen();

    while (true) {
        HttpConnection conn = listener.accept();
        HttpRequest request = conn.read_request();
        HttpResponse response = handler->handle_request(request);
        conn.write_response(response);
    }
}


