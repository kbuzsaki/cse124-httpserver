#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <memory>
#include <thread>
#include <vector>
#include "server.h"
#include "synchronized_queue.h"


/*
 * BlockingHttpConnectionHandler handles incoming connections on the calling thread,
 * blocking until the connection closes.
 */
class BlockingHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;

public:
    BlockingHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>);

    virtual void handle_connection(HttpConnection&&);
};


/*
 * ThreadSpawningHttpConnectionHandler handles incoming connections by spawning a new thread
 * for the incoming connection. This implements a "thread-per-connection" model. It returns
 * to the calling thread immediately.
 */
class ThreadSpawningHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;

public:
    ThreadSpawningHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>);

    virtual void handle_connection(HttpConnection&&);
};


/*
 * ThreadPoolHttpConnectionHandler handles incoming connections by passing them off to a pool
 * of `size` threads using a synchronized work queue. It returns to the calling thread
 * immediately, but the request will not begin processing until a worker thread is free to
 * pull the request off the queue.
 */
class ThreadPoolHttpConnectionHandler : public HttpConnectionHandler {
    std::shared_ptr<HttpRequestHandler> handler;
    std::vector<std::thread> thread_pool;
    std::shared_ptr<SynchronizedQueue<std::shared_ptr<HttpConnection>>> work_queue;

public:
    ThreadPoolHttpConnectionHandler(std::shared_ptr<HttpRequestHandler>, int size);

    virtual void handle_connection(HttpConnection&&);
};

#endif //CONNECTION_HANDLER_H
