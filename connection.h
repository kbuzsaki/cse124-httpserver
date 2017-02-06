#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sstream>


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
