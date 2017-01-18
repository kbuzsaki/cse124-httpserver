#ifndef LISTENER_H
#define LISTENER_H

#include <cstdint>
#include <string>
#include <sstream>
#include "connection.h"

class Listener {
public:
    virtual ~Listener() {};

    virtual void listen() = 0;
    virtual Connection* accept() = 0;
};


class SocketListener : public Listener {
    int sock;

public:
    SocketListener(uint16_t port);
    SocketListener(SocketListener&&);
    ~SocketListener();

    void listen();
    Connection* accept();
};

#endif // LISTENER_H