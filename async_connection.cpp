#include <iostream>
#include <poll.h>
#include "async_connection.h"
#include "connection.h"
#include "util.h"

using std::function;
using std::make_shared;
using std::shared_ptr;
using std::string;

#define INVALID_SOCK (-1)
#define BUFFER_SIZE (2000)


class SocketReadPollable : public Pollable {
    SocketAsyncConnection& conn;
    Callback<string>::F callback;
    bool is_done;

public:
    SocketReadPollable(SocketAsyncConnection& conn, Callback<string>::F callback) : conn(conn), callback(callback), is_done(false) {}

    virtual int get_fd() {
        return conn.client_sock;
    }

    virtual short get_events() {
        return POLLIN;
    }

    virtual bool done() {
        return is_done;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        is_done = true;
        return callback(conn.inner_read());
    }
};


class SocketWritePollable : public Pollable {
    SocketAsyncConnection& conn;
    string message;
    Callback<>::F callback;
    bool is_done;

public:
    SocketWritePollable(SocketAsyncConnection& conn, string message, Callback<>::F callback)
            : conn(conn), message(message), callback(callback), is_done(false) {}

    virtual int get_fd() {
        return conn.client_sock;
    }

    virtual short get_events() {
        return POLLOUT;
    }

    virtual bool done() {
        return is_done;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        is_done = true;
        conn.inner_write(message);
        return callback();
    }
};



SocketAsyncConnection::SocketAsyncConnection(int client_sock, struct in_addr client_remote_ip)
        : client_sock(client_sock), client_remote_ip(client_remote_ip) {
    std::cerr << "Initialized socket for ip: " << this->client_remote_ip << std::endl;
}



std::shared_ptr<Pollable> SocketAsyncConnection::read(Callback<string>::F callback) {
    return make_shared<SocketReadPollable>(*this, callback);
}

std::shared_ptr<Pollable> SocketAsyncConnection::write(string msg, Callback<>::F callback) {
    return make_shared<SocketWritePollable>(*this, msg, callback);
}

std::string SocketAsyncConnection::inner_read() {
    char buf[BUFFER_SIZE];

    ssize_t received = ::recv(client_sock, buf, sizeof(buf), 0);
    if (received < 0) {
        if (errno == EAGAIN) {
            // TODO: add test for this timeout?
            throw ConnectionClosed();
        } else {
            throw ConnectionError(errno_message("recv() failed: "));
        }
    } else if (received == 0) {
        throw ConnectionClosed();
    }

    buf[received] = '\0';
    return string(buf);
}

// TODO: handle writes larger than the buffer size?
void SocketAsyncConnection::inner_write(std::string s) {
    ssize_t sent = ::send(this->client_sock, s.c_str(), s.size(), 0);
    if (sent < 0) {
        throw ConnectionError(errno_message("send() failed: "));
    } else if ((size_t) sent != s.size()) {
        std::cerr << "WARNING: send() sent fewer bytes than expected!" << std::endl;
    }
}
