#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <memory>
#include <thread>
#include <vector>
#include "server.h"
#include "synchronized_queue.h"


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


class ThreadPoolHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;
    std::vector<std::thread> thread_pool;
    std::shared_ptr<SynchronizedQueue<std::shared_ptr<HttpConnection>>> work_queue;

public:
    ThreadPoolHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>, int size);

    virtual void handle_connection(HttpConnection&&);
};

#endif //CONNECTION_HANDLER_H
