#ifndef ASYNC_CONNECTION_H
#define ASYNC_CONNECTION_H

#include <functional>
#include <netinet/in.h>
#include "async_event_loop.h"

class SocketReadPollable;
class SocketWritePollable;

class SocketAsyncConnection {
    friend class SocketReadPollable;
    friend class SocketWritePollable;

    int client_sock;
    struct in_addr client_remote_ip;

    std::string inner_read();
    void inner_write(std::string);
public:
    SocketAsyncConnection(int client_sock, struct in_addr client_remote_ip);

    virtual std::shared_ptr<Pollable> read(Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> write(std::string, Callback<>::F callback);
};


#endif //ASYNC_CONNECTION_H
