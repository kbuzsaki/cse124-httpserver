#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <memory>
#include "server.h"


class BlockingHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;

public:
    BlockingHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>);

    virtual void handle_connection(HttpConnection&&);
};


class ThreadSpawningHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;

public:
    ThreadSpawningHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>);

    virtual void handle_connection(HttpConnection&&);
};

#endif //CONNECTION_HANDLER_H
