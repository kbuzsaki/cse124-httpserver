#include "server.h"
#include <iostream>

using std::shared_ptr;

#define CRLFCRLF ("\r\n\r\n")


HttpConnection::HttpConnection(shared_ptr<Connection> conn) : conn(conn) {}

HttpConnection::HttpConnection(HttpConnection&& http_conn) : conn(std::move(http_conn.conn)) {}

HttpFrame HttpConnection::read_frame() {
    return HttpFrame{conn.read_until(CRLFCRLF)};
}

void HttpConnection::write_frame(HttpFrame frame) {
    this->conn.write(frame.serialize());
}

HttpRequest HttpConnection::read_request() {
    HttpRequest request = parse_request_frame(this->read_frame());
    request.remote_ip = conn.remote_ip();
    return request;
}

void HttpConnection::write_response(HttpResponse response) {
    this->write_frame(response.pack());
}


HttpListener::HttpListener(shared_ptr<Listener> listener) : listener(listener) {}

HttpListener::HttpListener(HttpListener&& listener) : listener(listener.listener) {
    listener.listener = shared_ptr<Listener>();
}

void HttpListener::listen() {
    listener->listen();
}

HttpConnection HttpListener::accept() {
    return HttpConnection(listener->accept());
}


HttpServer::HttpServer(HttpListener&& listener, shared_ptr<HttpConnectionHandler> handler) : listener(std::move(listener)), handler(handler) {}

void HttpServer::serve() {
    listener.listen();

    while (true) {
        try {
            handler->handle_connection(listener.accept());
        } catch (ListenerError& e) {
            return;
        }
    }
}


