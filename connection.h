#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sstream>


/*
 * Connection is an abstract class that represents a bidirectional stream of bytes.
 * `read` will return a string of arbitrary size but at least length 1, or throw ConnectionClosed
 * `write` will accept a string of arbitrary size and block until it is sent, or throw ConnectionClosed
 *
 * Connection is implemented by the SocketConnection derived class below and the MockConnection class in mocks.h
 */
class Connection {
public:
    virtual ~Connection() {};

    virtual std::string read() = 0;
    virtual void write(std::string) = 0;
    virtual void close() = 0;
    virtual bool is_closed() = 0;

    virtual struct in_addr remote_ip() = 0;
};


class ConnectionError : public std::runtime_error {
public:
    ConnectionError(std::string);
};


class ConnectionClosed : public std::runtime_error {
public:
    ConnectionClosed();
};


/*
 * SocketConnection represents a bidirectional stream of bytes backed by a tcp socket.
 * `read` and `write` call `send` and `recv` on the underlying socket.
 * The ~SocketConnection() destructor shuts down and closes the socket.
 */
class SocketConnection : public Connection {
    int client_sock;
    struct in_addr client_remote_ip;

public:
    SocketConnection(int client_sock, struct in_addr client_remote_ip);
    SocketConnection(SocketConnection&&);
    virtual ~SocketConnection();

    virtual std::string read();
    virtual void write(std::string);
    virtual void close();
    virtual bool is_closed();

    virtual struct in_addr remote_ip();
};


/*
 * BufferedConnection is a convenience class that wraps a Connection and implements
 * buffered `read_until` operations that read until an arbitrary delimiter is encounterd.
 * It blocks until the delimiter is encountered and returns the string before the delimiter,
 * dropping the delimiter. If the underlying Connection closes, it throws ConnectionClosed.
 */
class BufferedConnection {
    std::shared_ptr<Connection> conn;
    std::stringstream buffer;

public:
    BufferedConnection();
    BufferedConnection(std::shared_ptr<Connection> conn);
    BufferedConnection(BufferedConnection&&);
    ~BufferedConnection();

    std::string read_until(std::string sep);
    void write(std::string body);
    void close();
    bool is_closed();

    struct in_addr remote_ip();
};

#endif // CONNECTION_H
