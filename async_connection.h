#ifndef ASYNC_CONNECTION_H
#define ASYNC_CONNECTION_H

#include <functional>
#include <netinet/in.h>
#include "async_event_loop.h"

class SocketReadPollable;
class SocketWritePollable;


class ConnectionHandle {
    int client_sock;
    struct in_addr client_remote_ip;

public:
    ConnectionHandle(int client_sock, struct in_addr client_remote_ip);
    ~ConnectionHandle();

    int get_fd();
    struct in_addr get_remote_ip();

    void close();

    std::string inner_read();
    void inner_write(std::string);
};


class AsyncSocketConnection {
    std::shared_ptr<ConnectionHandle> conn;

public:
    AsyncSocketConnection(int client_sock, struct in_addr client_remote_ip);

    virtual std::shared_ptr<Pollable> read(Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> write(std::string, Callback<>::F callback);
};


#endif //ASYNC_CONNECTION_H
