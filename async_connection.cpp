#include <iostream>
#include <poll.h>
#include "async_connection.h"
#include "util.h"


SocketAsyncConnection::SocketAsyncConnection(int client_sock, struct in_addr client_remote_ip)
        : client_sock(client_sock), client_remote_ip(client_remote_ip) {}

int SocketAsyncConnection::get_fd() {
    return client_sock;
}

short SocketAsyncConnection::get_events() {
    return POLLIN;
}

std::shared_ptr<Pollable> SocketAsyncConnection::notify(short) {
    std::cerr << "called client sock notify! (" << client_remote_ip << ")" << std::endl;
    return std::shared_ptr<Pollable>();
}

std::shared_ptr<Pollable> SocketAsyncConnection::read() {
    return std::shared_ptr<Pollable>();
}

std::shared_ptr<Pollable> SocketAsyncConnection::write() {
    return std::shared_ptr<Pollable>();
}
