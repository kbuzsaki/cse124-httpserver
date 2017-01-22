#include <exception>
#include <stdexcept>
#include "server.h"

using std::exception;
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


HttpServer::HttpServer(HttpListener&& listener, shared_ptr<HttpRequestHandler> handler) : listener(std::move(listener)), handler(handler) {}

void HttpServer::serve() {
    listener.listen();

    while (true) {
        try {
            HttpConnection conn = listener.accept();

            try {
                HttpRequest request = conn.read_request();
                if (!has_header(request.headers, "Host")) {
                    conn.write_response(bad_request_response());
                } else {
                    HttpResponse response = handler->handle_request(request);
                    conn.write_response(response);
                }
            } catch (HttpRequestParseError&) {
                conn.write_response(bad_request_response());
            } catch (exception&) {
                conn.write_response(internal_server_error_response());
            }
        } catch (ListenerError&) {
            // TODO: make this log somehow?
            return;
        }
    }
}


