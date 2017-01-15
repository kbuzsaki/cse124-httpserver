#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::string;
using std::stringstream;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

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
    close(this->sock);
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

    return BufferedConnection(client_sock);
}

BufferedConnection::BufferedConnection() : client_sock(INVALID_SOCK) {}

BufferedConnection::BufferedConnection(int client_sock) : client_sock(client_sock) {}

BufferedConnection::BufferedConnection(BufferedConnection&& conn) : client_sock(conn.client_sock), buffer(conn.buffer.str()) {
    conn.client_sock = INVALID_SOCK;
}

BufferedConnection::~BufferedConnection() {
    this->close();
}

bool BufferedConnection::is_closed() {
    return this->client_sock == INVALID_SOCK;
}

void BufferedConnection::close() {
    if (!this->is_closed()) {
        if (::shutdown(this->client_sock, SHUT_RDWR) < 0) {
            cerr << "shutdown() failed: " << strerror(errno) << endl;
        }
        if (::close(this->client_sock) < 0) {
            cerr << "close() failed: " << strerror(errno) << endl;
        }
    }
}

void BufferedConnection::write(string body) {
    if (!this->is_closed()) {
        send(this->client_sock, body.c_str(), body.size(), 0);
    }
}

// TODO: fix this to handle being closed
string BufferedConnection::read_until(string sep) {
    // if the separator is already in the buffer, return from the buffer
    size_t pos = this->buffer.str().find(sep, 0);
    if (pos != string::npos) {
        return pop_n_sstream(this->buffer, pos + sep.size());
    }
    
    // otherwise, read more from the socket
    char buf[BUFFER_SIZE];
    ssize_t received = 0;
    do {
        received = recv(client_sock, buf, sizeof(buf), 0);
        buf[received] = '\0';
        string buf_str = string(buf);
        
        size_t buf_pos = buf_str.find(sep, 0);
        if (buf_pos != string::npos) {
            size_t split_pos = sstream_size(this->buffer) + buf_pos;
            this->buffer << buf_str;
            return pop_n_sstream(this->buffer, split_pos);
        } else {
            this->buffer << buf_str;
        }
    } while (received > 0);
    
    return pop_n_sstream(this->buffer, sstream_size(this->buffer));
}





