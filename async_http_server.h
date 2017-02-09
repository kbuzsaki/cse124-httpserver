#ifndef ASYNC_HTTP_SERVER_H
#define ASYNC_HTTP_SERVER_H

#include "async_event_loop.h"
#include "async_listener.h"
#include "http.h"
#include <memory>


class AsyncHttpRequestHandler {
public:
    virtual ~AsyncHttpRequestHandler(){};

    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F callback) = 0;
};


class AsyncHttpServer {
    std::shared_ptr<AsyncSocketListener> listener;
    std::shared_ptr<AsyncHttpRequestHandler> handler;

public:
    AsyncHttpServer(std::shared_ptr<AsyncSocketListener> listener, std::shared_ptr<AsyncHttpRequestHandler> handler);

    void serve();
};

#endif //ASYNC_HTTP_SERVER_H
