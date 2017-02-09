#include "async_http_server.h"
#include "async_connection_handler.h"

using std::make_shared;
using std::shared_ptr;

AsyncHttpServer::AsyncHttpServer(std::shared_ptr<AsyncSocketListener> listener, std::shared_ptr<AsyncHttpRequestHandler> handler)
        : listener(listener), handler(handler) {}


void AsyncHttpServer::serve() {
    listener->listen();

    AsyncEventLoop loop;

    AsyncHttpConnectionHandler conn_handler(handler);

    loop.register_pollable(make_pollable(listener, [&conn_handler](shared_ptr<AsyncSocketConnection> conn) -> shared_ptr<Pollable> {
        return conn_handler.handle_socket_connection(make_shared<AsyncHttpConnection>(conn));
    }));

    loop.loop();
}
