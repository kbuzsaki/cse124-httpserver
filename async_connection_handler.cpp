#include <iostream>
#include "async_connection_handler.h"
#include "async_http_connection.h"
#include "http.h"
#include "server.h"

using std::make_shared;
using std::shared_ptr;
using std::string;


shared_ptr<Pollable> TestAsyncHttpRequestHandler::handle_request(HttpRequest, Callback<HttpResponse>::F callback) {
    return callback(forbidden_response());
}


shared_ptr<Pollable> make_request_handler(shared_ptr<AsyncHttpConnection> http_conn, shared_ptr<AsyncHttpRequestHandler> handler) {
    return http_conn->read_request([http_conn, handler](HttpRequest request) -> shared_ptr<Pollable> {
        return handler->handle_request(request, [http_conn, handler](HttpResponse response) -> shared_ptr<Pollable> {
            return http_conn->write_response(response, [http_conn, handler]() -> shared_ptr<Pollable> {
                return make_request_handler(http_conn, handler);
            });
        });
    });
}

shared_ptr<Pollable> handle_socket_connection(shared_ptr<AsyncSocketConnection> conn) {
    shared_ptr<AsyncHttpConnection> http_conn = make_shared<AsyncHttpConnection>(conn);
    shared_ptr<AsyncHttpRequestHandler> handler = make_shared<TestAsyncHttpRequestHandler>();

    return make_request_handler(http_conn, handler);
}

AsyncHttpConnectionHandler::AsyncHttpConnectionHandler(shared_ptr<AsyncHttpRequestHandler> handler) : handler(handler) {}

shared_ptr<Pollable> AsyncHttpConnectionHandler::handle_socket_connection(shared_ptr<AsyncHttpConnection> http_conn) {
    return make_request_handler(http_conn, handler);
}
