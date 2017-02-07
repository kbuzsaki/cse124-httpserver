#include <iostream>
#include "async_connection_handler.h"

std::function<std::shared_ptr<Pollable> (std::string)> make_read_cb(std::shared_ptr<SocketAsyncConnection> conn);
std::function<std::shared_ptr<Pollable> ()> make_write_cb(std::shared_ptr<SocketAsyncConnection> conn);


std::function<std::shared_ptr<Pollable> (std::string)> make_read_cb(std::shared_ptr<SocketAsyncConnection> conn) {
    return [conn](std::string s) -> std::shared_ptr<Pollable> {
        std::cerr << "in read callback" << std::endl;
        std::cerr << "got str: '" << s << "'" << std::endl;
        return conn->write(s, make_write_cb(conn));
    };
}

std::function<std::shared_ptr<Pollable> ()> make_write_cb(std::shared_ptr<SocketAsyncConnection> conn) {
    return [conn]() -> std::shared_ptr<Pollable> {
        std::cerr << "in write callback" << std::endl;
        return conn->read(make_read_cb(conn));
    };
}


std::shared_ptr<Pollable> handle_socket_connection(std::shared_ptr<SocketAsyncConnection> conn) {
    return conn->read(make_read_cb(conn));
}

