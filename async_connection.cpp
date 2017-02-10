#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include "async_connection.h"
#include "connection.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::ios;
using std::make_shared;
using std::shared_ptr;
using std::stringstream;
using std::string;

#define INVALID_SOCK (-1)
#define BUFFER_SIZE (2000)


ConnectionHandle::ConnectionHandle(int client_sock, struct in_addr client_remote_ip)
        : client_sock(client_sock), client_remote_ip(client_remote_ip) {}

ConnectionHandle::~ConnectionHandle() {
    this->close();
}

int ConnectionHandle::get_fd() {
    return client_sock;
}

struct in_addr ConnectionHandle::get_remote_ip() {
    return client_remote_ip;
}

void ConnectionHandle::close() {
    // ignore ENOTCONN in case the client closes before us
    if (::shutdown(this->client_sock, SHUT_RDWR) < 0  && errno != ENOTCONN) {
        cerr << errno_message("shutdown() failed: ") << endl;
    }
    if (::close(this->client_sock) < 0) {
        cerr << errno_message("close() failed: ") << endl;
    }
    this->client_sock = INVALID_SOCK;
}

std::string ConnectionHandle::inner_read() {
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
void ConnectionHandle::inner_write(std::string s) {
    ssize_t sent = ::send(this->client_sock, s.c_str(), s.size(), 0);
    if (sent < 0) {
        throw ConnectionError(errno_message("send() failed: "));
    } else if ((size_t) sent != s.size()) {
        std::cerr << "WARNING: send() sent fewer bytes than expected!" << std::endl;
    }
}


class SocketReadPollable : public Pollable {
    shared_ptr<ConnectionHandle> conn;
    Callback<string>::F callback;
    bool done;

public:
    SocketReadPollable(shared_ptr<ConnectionHandle> conn, Callback<string>::F callback) : conn(conn), callback(callback), done(false) {}

    virtual int get_fd() {
        return conn->get_fd();
    }

    virtual short get_events() {
        return POLLIN;
    }

    virtual bool is_done() {
        return done;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        done = true;
        return callback(conn->inner_read());
    }
};


class SocketWritePollable : public Pollable {
    shared_ptr<ConnectionHandle> conn;
    string message;
    Callback<>::F callback;
    bool done;

public:
    SocketWritePollable(shared_ptr<ConnectionHandle> conn, string message, Callback<>::F callback)
            : conn(conn), message(message), callback(callback), done(false) {}

    virtual int get_fd() {
        return conn->get_fd();
    }

    virtual short get_events() {
        return POLLOUT;
    }

    virtual bool is_done() {
        return done;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        done = true;
        conn->inner_write(message);
        return callback();
    }
};


AsyncSocketConnection::AsyncSocketConnection(int client_sock, struct in_addr client_remote_ip)
        : conn(make_shared<ConnectionHandle>(client_sock, client_remote_ip)) {}

struct in_addr AsyncSocketConnection::get_remote_ip() {
    return conn->get_remote_ip();
}

std::shared_ptr<Pollable> AsyncSocketConnection::read(Callback<string>::F callback) {
    return make_shared<SocketReadPollable>(conn, callback);
}

std::shared_ptr<Pollable> AsyncSocketConnection::write(string msg, Callback<>::F callback) {
    return make_shared<SocketWritePollable>(conn, msg, callback);
}


AsyncBufferedConnection::AsyncBufferedConnection(std::shared_ptr<AsyncSocketConnection> conn)
        : conn(conn), buffer(make_shared<stringstream>()) {}

Callback<string>::F make_read_into_buffer(shared_ptr<AsyncSocketConnection> conn, shared_ptr<stringstream> buffer, string sep, Callback<string>::F callback) {
    return [=](string buf_str) -> shared_ptr<Pollable> {
        buffer->seekp(0, ios::end);
        (*buffer) << buf_str;

        // TODO: maybe optimize this?
        size_t split_pos = buffer->str().find(sep, 0);
        if (split_pos != string::npos) {
            string content = pop_n_sstream(*buffer, split_pos, sep.size());
            return callback(content);
        } else {
            return conn->read(make_read_into_buffer(conn, buffer, sep, callback));
        }
    };
}

std::shared_ptr<Pollable> AsyncBufferedConnection::read_until(std::string sep, Callback<std::string>::F callback) {
    // first try to read from the buffer by checking for the separator
    size_t pos = buffer->str().find(sep, 0);
    if (pos != string::npos) {
        string content = pop_n_sstream(*buffer, pos, sep.size());
        return callback(content);
    }

    return conn->read(make_read_into_buffer(conn, buffer, sep, callback));
}

std::shared_ptr<Pollable> AsyncBufferedConnection::write(std::string s, Callback<>::F callback) {
    return conn->write(s, callback);
}

struct in_addr AsyncBufferedConnection::get_remote_ip() {
    return conn->get_remote_ip();
}
