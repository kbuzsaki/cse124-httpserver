#ifndef ASYNC_CONNECTION_H
#define ASYNC_CONNECTION_H

#include <netinet/in.h>
#include "async_event_loop.h"

class AsyncConnection : public Pollable {
public:
    virtual std::shared_ptr<Pollable> read() = 0;
    virtual std::shared_ptr<Pollable> write() = 0;
};

class SocketAsyncConnection : public AsyncConnection {
    int client_sock;
    struct in_addr client_remote_ip;

public:
    SocketAsyncConnection(int client_sock, struct in_addr client_remote_ip);

    virtual int get_fd();
    virtual short get_events();

    virtual std::shared_ptr<Pollable> notify(short revents);

    virtual std::shared_ptr<Pollable> read();
    virtual std::shared_ptr<Pollable> write();
};


#endif //ASYNC_CONNECTION_H
