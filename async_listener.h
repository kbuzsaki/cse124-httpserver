#ifndef ASYNC_LISTENER_H_H
#define ASYNC_LISTENER_H_H

#include <memory>
#include "async_connection.h"
#include "async_event_loop.h"


class AsyncSocketListener : public Pollable {
    int sock;

public:
    AsyncSocketListener(uint16_t port);
    AsyncSocketListener(AsyncSocketListener&&);
    ~AsyncSocketListener();

    void listen();
    std::shared_ptr<SocketAsyncConnection> accept();

    virtual int get_fd();
    virtual short get_events();
    virtual bool done();
    virtual std::shared_ptr<Pollable> notify(short revents);
};


#endif //ASYNC_LISTENER_H_H
