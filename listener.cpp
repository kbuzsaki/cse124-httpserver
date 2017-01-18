#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "listener.h"
#include "util.h"

using std::cerr;
using std::endl;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (100)

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

SocketListener::SocketListener(SocketListener&& listener) : sock(listener.sock) {
    listener.sock = INVALID_SOCK;
}

SocketListener::~SocketListener() {
    // TODO: add a proper notion of "closed" to socket listener
    if (sock != INVALID_SOCK) {
        ::shutdown(this->sock, SHUT_RDWR);
        ::close(this->sock);
    }
}

void SocketListener::listen() {
    int err = ::listen(sock, QUEUE_SIZE);
    if (err < 0) {
        cerr << "listen() failed: " << strerror(errno) << endl;
        return;
    }
}

Connection* SocketListener::accept() {
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    int client_sock = ::accept(sock, (struct sockaddr*) &client_addr, &client_len);
    if (client_sock < 0) {
        cerr << "accept() failed: " << strerror(errno) << endl;
    }

    return new SocketConnection(client_sock);
}
