#include "async_http_server.h"
#include "async_http_connection.h"

using std::make_shared;
using std::shared_ptr;


shared_ptr<Pollable> handle_http_connection(shared_ptr<AsyncHttpConnection> http_conn, shared_ptr<AsyncHttpRequestHandler> handler);


AsyncHttpServer::AsyncHttpServer(std::shared_ptr<AsyncSocketListener> listener, std::shared_ptr<AsyncHttpRequestHandler> handler)
        : listener(listener), handler(handler) {}

void AsyncHttpServer::serve() {
    AsyncEventLoop loop;

    // begin listening and register a handler for incoming connections
    listener->listen();
    loop.register_pollable(make_pollable(listener, [=](shared_ptr<AsyncSocketConnection> conn) -> shared_ptr<Pollable> {
        return handle_http_connection(make_shared<AsyncHttpConnection>(conn), handler);
    }));

    loop.loop();
}


shared_ptr<Pollable> handle_http_connection(shared_ptr<AsyncHttpConnection> http_conn, shared_ptr<AsyncHttpRequestHandler> handler) {
    return http_conn->read_request([=](HttpRequest request) -> shared_ptr<Pollable> {
        if (!has_header(request.headers, "Host")) {
            return http_conn->write_response(bad_request_response(), Callback<>::empty());
        }

        return handler->handle_request(request, [=](HttpResponse response) -> shared_ptr<Pollable> {
            return http_conn->write_response(response, [=]() -> shared_ptr<Pollable> {
                if (get_header(request.headers, "Connection").value.find("close") != std::string::npos) {
                    return shared_ptr<Pollable>();
                }

                return handle_http_connection(http_conn, handler);
            });
        });
    });
}
