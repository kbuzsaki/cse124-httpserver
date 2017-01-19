#ifndef LISTENER_H
#define LISTENER_H

#include <cstdint>
#include <exception>
#include <string>
#include <sstream>
#include "connection.h"

class Listener {
public:
    virtual ~Listener() {};

    virtual void listen() = 0;
    virtual std::shared_ptr<Connection> accept() = 0;
};


class ListenerError : public std::runtime_error {
public:
    ListenerError(std::string);
};


class SocketListener : public Listener {
    int sock;

public:
    SocketListener(uint16_t port);
    SocketListener(SocketListener&&);
    ~SocketListener();

    void listen();
    std::shared_ptr<Connection> accept();
};

#endif // LISTENER_H
