#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::ios;
using std::string;
using std::stringstream;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

// TODO: add proper error handling via exceptions when connection stuff fails

SocketListener::SocketListener(uint16_t port) {
    this->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in target;
    bzero(&target, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = htonl(INADDR_ANY);
    target.sin_port = htons(port);

    int err = bind(this->sock, (struct sockaddr*) &target, sizeof(target));
    if (err < 0) {
        cerr << "bind() failed: " << strerror(errno) << endl;
        return;
    }
}

SocketListener::~SocketListener() {
    ::shutdown(this->sock, SHUT_RDWR);
    ::close(this->sock);
}

void SocketListener::listen() {
    int err = ::listen(sock, QUEUE_SIZE);
    if (err < 0) {
        cerr << "listen() failed: " << strerror(errno) << endl;
        return;
    }
}

BufferedConnection SocketListener::accept() {
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    int client_sock = ::accept(sock, (struct sockaddr*) &client_addr, &client_len);
    if (client_sock < 0) {
        cerr << "accept() failed: " << strerror(errno) << endl;
    }

    return BufferedConnection(new SocketConnection(client_sock));
}


SocketConnection::SocketConnection() : client_sock(INVALID_SOCK) {}

SocketConnection::SocketConnection(int client_sock) : client_sock(client_sock) {}

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
        cerr << "recv() failed: " << strerror(errno) << endl;
        return "";
    }

    buf[received] = '\0';
    return string(buf);
}

void SocketConnection::write(std::string s) {
    if (!this->is_closed()) {
        ::send(this->client_sock, s.c_str(), s.size(), 0);
    }
}

void SocketConnection::close() {
    if (!this->is_closed()) {
        if (::shutdown(this->client_sock, SHUT_RDWR) < 0) {
            cerr << "shutdown() failed: " << strerror(errno) << endl;
        }
        if (::close(this->client_sock) < 0) {
            cerr << "close() failed: " << strerror(errno) << endl;
        }
        this->client_sock = INVALID_SOCK;
    }
}

bool SocketConnection::is_closed() {
    return this->client_sock == INVALID_SOCK;
}


BufferedConnection::BufferedConnection() : conn(NULL), buffer() {}

BufferedConnection::BufferedConnection(Connection* conn) : conn(conn), buffer() {}

BufferedConnection::BufferedConnection(BufferedConnection&& conn) : conn(conn.conn), buffer(conn.buffer.str()) {
    conn.conn = NULL;
}

BufferedConnection::~BufferedConnection() {
    this->close();
    delete this->conn;
}

bool BufferedConnection::is_closed() {
    // TODO: is this acceptable behavior?
    return conn == NULL || conn->is_closed();
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



