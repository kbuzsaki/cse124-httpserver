#include "server.h"

HttpListener::HttpListener(SocketListener&& listener) : listener(std::move(listener)) {}

HttpListener::HttpListener(HttpListener&& listener) : listener(std::move(listener.listener)) {}

void HttpListener::listen() {
    listener.listen();
}

HttpConnection HttpListener::accept() {
    return HttpConnection(listener.accept());
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


