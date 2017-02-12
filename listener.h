#ifndef LISTENER_H
#define LISTENER_H

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include "connection.h"

/*
 * Listener represents a listen server that will accept incoming connections.
 * It is implemented by SocketListener below and MockListener in mocks.h
 */
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


/*
 * Socket listener listens on the given port on the wildcard interface and returns
 * pointers to incoming connections when the `accept` method is called.
 * If no connection is waiting when `accept` is called, it blocks until one is ready.
 * The ~SocketListener destructor closes the listening socket.
 */
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
