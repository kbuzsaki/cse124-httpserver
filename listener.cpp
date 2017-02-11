#include <unistd.h>
#include <sys/socket.h>
#include <sstream>
#include "string.h"
#include <netinet/in.h>
#include "listener.h"
#include "util.h"

using std::make_shared;
using std::shared_ptr;
using std::stringstream;

#define INVALID_SOCK (-1)
#define QUEUE_SIZE (2000)

ListenerError::ListenerError(std::string message) : runtime_error(message) {}


SocketListener::SocketListener(uint16_t port) {
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

SocketListener::SocketListener(SocketListener&& listener) : sock(listener.sock) {
    listener.sock = INVALID_SOCK;
}

SocketListener::~SocketListener() {
    if (sock != INVALID_SOCK) {
        ::shutdown(this->sock, SHUT_RDWR);
        ::close(this->sock);
    }
}

void SocketListener::listen() {
    int err = ::listen(sock, QUEUE_SIZE);
    if (err < 0) {
        throw ListenerError(errno_message("listen() failed: "));
    }
}

shared_ptr<Connection> SocketListener::accept() {
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    int client_sock = ::accept(sock, (struct sockaddr*) &client_addr, &client_len);
    if (client_sock < 0) {
        throw ListenerError(errno_message("accept() failed: "));
    }

    return make_shared<SocketConnection>(client_sock, client_addr.sin_addr);
}
