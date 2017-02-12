#ifndef ASYNC_HTTP_CONNECTION_H
#define ASYNC_HTTP_CONNECTION_H

#include <functional>
#include "async_event_loop.h"
#include "http.h"
#include "async_connection.h"


/*
 * AsyncHttpConnection wraps an AsyncSocketConnection and provides non-blocking
 * read_request and write_response methods that invoke their callbacks when
 * the operation is complete.
 */
class AsyncHttpConnection {
    AsyncBufferedConnection conn;

public:
    AsyncHttpConnection(std::shared_ptr<AsyncSocketConnection> conn);

    std::shared_ptr<Pollable> read_request(Callback<HttpRequest>::F callback);
    std::shared_ptr<Pollable> write_response(HttpResponse, Callback<>::F callback);
};

#endif //ASYNC_HTTP_CONNECTION_H
