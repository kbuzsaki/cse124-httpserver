#ifndef ASYNC_HTTP_SERVER_H
#define ASYNC_HTTP_SERVER_H

#include "async_event_loop.h"
#include "async_listener.h"
#include "http.h"
#include <memory>


/*
 * AsyncHttpRequestHandler is an abstract class that represents the minimal interface
 * for asynchronously handling an http request.
 * `handle_request` accepts an HttpRequest and a callback to invoke when the response is ready.
 */
class AsyncHttpRequestHandler {
public:
    virtual ~AsyncHttpRequestHandler(){};

    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F callback) = 0;
};


/*
 * AsyncHttpServer takes an AsyncSocketListener and AsyncHttpRequestHandler and creates
 * and runs an AsyncEventLoop processing connections read from the AsyncSocketListener
 * with the given AsyncHttpRequestHandler.
 */
class AsyncHttpServer {
    std::shared_ptr<AsyncSocketListener> listener;
    std::shared_ptr<AsyncHttpRequestHandler> handler;

public:
    AsyncHttpServer(std::shared_ptr<AsyncSocketListener> listener, std::shared_ptr<AsyncHttpRequestHandler> handler);

    void serve();
};

#endif //ASYNC_HTTP_SERVER_H
