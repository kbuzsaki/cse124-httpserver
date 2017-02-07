#ifndef ASYNC_HTTP_CONNECTION_H
#define ASYNC_HTTP_CONNECTION_H

#include <functional>
#include "async_event_loop.h"
#include "http.h"

class AsyncHttpConnection {

public:

    std::shared_ptr<Pollable> read_request(std::function<std::shared_ptr<Pollable> (HttpRequest)> callback);
    std::shared_ptr<Pollable> write_response(std::function<std::shared_ptr<Pollable> ()> callback);
};

#endif //ASYNC_HTTP_CONNECTION_H
