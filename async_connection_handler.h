#ifndef ASYNC_CONNECTION_HANDLER_H
#define ASYNC_CONNECTION_HANDLER_H

#include "async_event_loop.h"
#include "async_connection.h"
#include "async_http_server.h"
#include "async_http_connection.h"
#include "http.h"


class TestAsyncHttpRequestHandler : public AsyncHttpRequestHandler {
public:
    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F done);
};


class AsyncHttpConnectionHandler {
    std::shared_ptr<AsyncHttpRequestHandler> handler;

public:
    AsyncHttpConnectionHandler(std::shared_ptr<AsyncHttpRequestHandler> handler);

    std::shared_ptr<Pollable> handle_socket_connection(std::shared_ptr<AsyncHttpConnection> http_conn);
};


std::shared_ptr<Pollable> handle_socket_connection(std::shared_ptr<AsyncSocketConnection> conn);

#endif //ASYNC_CONNECTION_HANDLER_H
