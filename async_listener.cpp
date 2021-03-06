#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <sstream>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include "async_listener.h"
#include "listener.h"
#include "util.h"

using std::make_shared;
using std::shared_ptr;
using std::chrono::system_clock;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (2000)


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
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    int client_sock = ::accept(sock, (struct sockaddr*) &client_addr, &client_len);
    if (client_sock < 0) {
        throw ListenerError(errno_message("accept() failed: "));
    }

    int ret = fcntl(client_sock, F_SETFL, fcntl(client_sock, F_GETFL, 0) | O_NONBLOCK);
    if (ret < 0) {
        std::cerr << "unable to set client sock to non blocking!" << std::endl;
    }

    return make_shared<AsyncSocketConnection>(client_sock, client_addr.sin_addr);
}

int AsyncSocketListener::get_fd() {
    return sock;
}


/*
 * AsyncSocketListenerPollable represents a non-blocking listen on the given
 * AsyncSocketListener. When the socket listener is ready, it calls accept() and invokes its callback
 * with the resulting connection.
 */
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

    virtual bool is_done() {
        return false;
    }

    virtual bool past_deadline(system_clock::time_point) {
        return false;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        return callback(listener->accept());
    }
};

shared_ptr<Pollable> make_pollable(shared_ptr<AsyncSocketListener> listener, Callback<shared_ptr<AsyncSocketConnection>>::F callback) {
    return make_shared<AsyncSocketListenerPollable>(listener, callback);
}

