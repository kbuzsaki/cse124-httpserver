#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include "connection.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::ios;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::stringstream;

#define INVALID_SOCK (-1)
#define BUFFER_SIZE (2000)

// TODO: add proper error handling via exceptions when connection stuff fails

ConnectionError::ConnectionError(string message) : runtime_error(message) {}

ConnectionClosed::ConnectionClosed() : runtime_error("connection closed") {}


SocketConnection::SocketConnection(int client_sock, struct in_addr client_remote_ip) : client_sock(client_sock), client_remote_ip(client_remote_ip) {
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int err = setsockopt(this->client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (err < 0) {
        cerr << errno_message("setsockopt() failed: ") << endl;
    }
}

SocketConnection::SocketConnection(SocketConnection&& conn) : client_sock(conn.client_sock) {
    conn.client_sock = INVALID_SOCK;
}

SocketConnection::~SocketConnection() {
    this->close();
}

string SocketConnection::read() {
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

void SocketConnection::write(std::string s) {
    if (!this->is_closed()) {
        ssize_t sent = ::send(this->client_sock, s.c_str(), s.size(), 0);
        if (sent < 0) {
            throw ConnectionError(errno_message("send() failed: "));
        } else if ((size_t) sent != s.size()) {
            cerr << "WARNING: send() sent fewer bytes than expected!" << endl;
        }
    }
}

void SocketConnection::close() {
    if (!this->is_closed()) {
        // ignore ENOTCONN in case the client closes before us
        if (::shutdown(this->client_sock, SHUT_RDWR) < 0  && errno != ENOTCONN) {
            cerr << errno_message("shutdown() failed: ") << endl;
        }
        if (::close(this->client_sock) < 0) {
            cerr << errno_message("close() failed: ") << endl;
        }
        this->client_sock = INVALID_SOCK;
    }
}

bool SocketConnection::is_closed() {
    return this->client_sock == INVALID_SOCK;
}

struct in_addr SocketConnection::remote_ip() {
    return client_remote_ip;
}


BufferedConnection::BufferedConnection() : conn(), buffer() {}

BufferedConnection::BufferedConnection(shared_ptr<Connection> conn) : conn(conn), buffer() {}

BufferedConnection::BufferedConnection(BufferedConnection&& conn) : conn(conn.conn), buffer(conn.buffer.str()) {
    conn.conn = shared_ptr<Connection>();
}

BufferedConnection::~BufferedConnection() {
    this->close();
}

bool BufferedConnection::is_closed() {
    // TODO: is this acceptable behavior?
    return !conn || conn->is_closed();
}

void BufferedConnection::close() {
    // TODO: is this acceptable behavior?
    if (!is_closed()) {
        conn->close();
    }
}

void BufferedConnection::write(string s) {
    conn->write(s);
}

string BufferedConnection::read_until(string sep) {
    // first try to read from the buffer by checking for the separator
    size_t pos = this->buffer.str().find(sep, 0);
    if (pos != string::npos) {
        return pop_n_sstream(this->buffer, pos, sep.size());
    }

    // otherwise, read more from the inner connection
    string buf_str;
    do {
        buf_str = conn->read();
        this->buffer.seekp(0, ios::end);
        this->buffer << buf_str;

        // TODO: maybe optimize this?
        size_t split_pos = this->buffer.str().find(sep, 0);
        if (split_pos != string::npos) {
            return pop_n_sstream(this->buffer, split_pos, sep.size());
        }
    } while (buf_str != "");

    // if we get a failed read (empty string), just give up and return what's in the buffer
    return pop_n_sstream(this->buffer, sstream_size(this->buffer), 0);
}

struct in_addr BufferedConnection::remote_ip() {
    return conn->remote_ip();
}



