#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <string>
#include <sstream>


class Connection {
public:
    virtual ~Connection() {};

    virtual std::string read() = 0;
    virtual void write(std::string) = 0;
    virtual void close() = 0;
    virtual bool is_closed() = 0;
};


class SocketConnection : public Connection {
    int client_sock;

public:
    SocketConnection();
    SocketConnection(int client_sock);
    SocketConnection(SocketConnection&&);
    virtual ~SocketConnection();

    virtual std::string read();
    virtual void write(std::string);
    virtual void close();
    virtual bool is_closed();
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
};

#endif // CONNECTION_H
