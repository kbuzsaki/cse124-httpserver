#include <iostream>
#include "async_connection_handler.h"

Callback<std::string>::F make_read_cb(std::shared_ptr<AsyncSocketConnection> conn);
Callback<>::F make_write_cb(std::shared_ptr<AsyncSocketConnection> conn);


Callback<std::string>::F make_read_cb(std::shared_ptr<AsyncSocketConnection> conn) {
    return [conn](std::string s) -> std::shared_ptr<Pollable> {
        std::cerr << "in read callback" << std::endl;
        std::cerr << "got str: '" << s << "'" << std::endl;
        return conn->write(s, make_write_cb(conn));
    };
}

Callback<>::F make_write_cb(std::shared_ptr<AsyncSocketConnection> conn) {
    return [conn]() -> std::shared_ptr<Pollable> {
        std::cerr << "in write callback" << std::endl;
        return conn->read(make_read_cb(conn));
    };
}


std::shared_ptr<Pollable> handle_socket_connection(std::shared_ptr<AsyncSocketConnection> conn) {
    return conn->read(make_read_cb(conn));
}

