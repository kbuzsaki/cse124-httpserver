#include <iostream>
#include <sys/socket.h>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include "async_listener.h"
#include "listener.h"
#include "util.h"

using std::make_shared;
using std::shared_ptr;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (100)


AsyncSocketListener::AsyncSocketListener(uint16_t port) {
    this->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in target;
    bzero(&target, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = htonl(INADDR_ANY);
    target.sin_port = htons(port);

    int err = bind(this->sock, (struct sockaddr*) &target, sizeof(target));
    if (err < 0) {
        throw ListenerError(errno_message("bind() failed: "));
    }
}

AsyncSocketListener::AsyncSocketListener(AsyncSocketListener&& listener) : sock(listener.sock) {
    listener.sock = INVALID_SOCK;
}

AsyncSocketListener::~AsyncSocketListener() {
    // TODO: add a proper notion of "closed" to socket listener
    if (sock != INVALID_SOCK) {
        ::shutdown(this->sock, SHUT_RDWR);
        ::close(this->sock);
    }
}

void AsyncSocketListener::listen() {
    int err = ::listen(sock, QUEUE_SIZE);
    if (err < 0) {
        throw ListenerError(errno_message("listen() failed: "));
    }
}

shared_ptr<AsyncSocketConnection> AsyncSocketListener::accept() {
    std::cerr << "called async accept! " << std::endl;
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    int client_sock = ::accept(sock, (struct sockaddr*) &client_addr, &client_len);
    if (client_sock < 0) {
        throw ListenerError(errno_message("accept() failed: "));
    }

    return make_shared<AsyncSocketConnection>(client_sock, client_addr.sin_addr);
}

int AsyncSocketListener::get_fd() {
    return sock;
}


class AsyncSocketListenerPollable : public Pollable {
    shared_ptr<AsyncSocketListener> listener;
    Callback<shared_ptr<AsyncSocketConnection>>::F callback;

public:
    AsyncSocketListenerPollable(shared_ptr<AsyncSocketListener> listener, Callback<shared_ptr<AsyncSocketConnection>>::F callback)
            : listener(listener), callback(callback) {}

    virtual int get_fd() {
        return listener->get_fd();
    }

    virtual short get_events() {
        return POLLIN;
    }

    virtual bool done() {
        return false;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        return callback(listener->accept());
    }
};

shared_ptr<Pollable> make_pollable(shared_ptr<AsyncSocketListener> listener, Callback<shared_ptr<AsyncSocketConnection>>::F callback) {
    return make_shared<AsyncSocketListenerPollable>(listener, callback);
}

