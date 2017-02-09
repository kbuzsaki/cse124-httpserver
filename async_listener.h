#ifndef ASYNC_LISTENER_H_H
#define ASYNC_LISTENER_H_H

#include <memory>
#include "async_connection.h"
#include "async_event_loop.h"


class AsyncSocketListener {
    int sock;

public:
    AsyncSocketListener(uint16_t port);
    AsyncSocketListener(AsyncSocketListener&&);
    ~AsyncSocketListener();

    void listen();
    std::shared_ptr<AsyncSocketConnection> accept();

    int get_fd();
};

std::shared_ptr<Pollable> make_pollable(std::shared_ptr<AsyncSocketListener> listener, Callback<std::shared_ptr<AsyncSocketConnection>>::F conn);


#endif //ASYNC_LISTENER_H_H
