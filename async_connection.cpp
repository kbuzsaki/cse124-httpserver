#include <algorithm>
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
#define BUFSIZE (1024 * 1024)

// TODO: clean up this bit of the code, make it actually non-blocking


class SocketReadPollable : public Pollable {
    int sock;
    Callback<string>::F callback;
    bool done;

    string try_read() {
        char buf[BUFSIZE];

        ssize_t received = ::recv(sock, buf, sizeof(buf), 0);
        if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return "";
        } else if (received < 0) {
            throw ConnectionError(errno_message("recv() failed: "));
        } else if (received == 0) {
            throw ConnectionClosed();
        }

        buf[received] = '\0';
        return string(buf, (size_t)received);
    }

public:
    SocketReadPollable(int sock, Callback<string>::F callback) : sock(sock), callback(callback), done(false) {}

    virtual int get_fd() {
        return sock;
    }

    virtual short get_events() {
        return POLLIN;
    }

    virtual bool is_done() {
        return done;
    }

    virtual std::shared_ptr<Pollable> notify(short) {
        string s = try_read();
        if (s != "") {
            done = true;
        }
        return callback(s);
    }
};


class SocketWritePollable : public Pollable {
    int sock;
    Callback<>::F callback;
    bool done;
    string message;
    size_t total_sent;

    bool try_write() {
        do {
            size_t left = message.size() - total_sent;
            size_t send_amount = std::min(left, (size_t)BUFSIZE);
            ssize_t sent = send(sock, message.c_str() + total_sent, send_amount, 0);

            if (sent < 0 && (errno == EAGAIN || EWOULDBLOCK)) {
                return false;
            } else if (sent < 0) {
                std::cerr << errno_message("send() failed: ") << std::endl;
                return false;
            }

            total_sent += sent;
        } while (total_sent < message.size());

        return true;
    }

public:
    SocketWritePollable(int sock, string message, Callback<>::F callback) : sock(sock), callback(callback), done(false), message(message), total_sent(0) {}

    virtual int get_fd() {
        return sock;
    }

    virtual short get_events() {
        return POLLOUT;
    }

    virtual bool is_done() {
        return done;
    }

    virtual shared_ptr<Pollable> notify(short) {
        done = try_write();
        if (done) {
            return callback();
        } else {
            return shared_ptr<Pollable>();
        }
    }
};


AsyncSocketConnection::AsyncSocketConnection(int client_sock, struct in_addr client_remote_ip)
        : client_sock(client_sock), client_remote_ip(client_remote_ip) {}

struct in_addr AsyncSocketConnection::get_remote_ip() {
    return client_remote_ip;
}

std::shared_ptr<Pollable> AsyncSocketConnection::read(Callback<string>::F callback) {
    return make_shared<SocketReadPollable>(client_sock, callback);
}

std::shared_ptr<Pollable> AsyncSocketConnection::write(string msg, Callback<>::F callback) {
    return make_shared<SocketWritePollable>(client_sock, msg, callback);
}

AsyncSocketConnection::AsyncSocketConnection(AsyncSocketConnection&& conn) : client_sock(conn.client_sock), client_remote_ip(conn.client_remote_ip) {
    conn.client_sock = INVALID_SOCK;
}

AsyncSocketConnection::~AsyncSocketConnection() {
    // ignore ENOTCONN in case the client closes before us
    if (::shutdown(this->client_sock, SHUT_RDWR) < 0  && errno != ENOTCONN) {
        cerr << errno_message("shutdown() failed: ") << endl;
    }
    if (::close(this->client_sock) < 0) {
        cerr << errno_message("close() failed: ") << endl;
    }
    this->client_sock = INVALID_SOCK;
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
