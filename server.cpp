#include "server.h"

using std::shared_ptr;


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
        } catch (ListenerError&) {
            // TODO: make this log somehow?
            return;
        }
    }
}


