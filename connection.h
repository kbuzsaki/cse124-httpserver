#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <string>
#include <sstream>

class BufferedConnection;

class SocketListener {
    int sock;
    
public:
    SocketListener(uint16_t port);
    ~SocketListener();
    
    void listen();
    BufferedConnection accept();
};

class BufferedConnection {
    int client_sock;
    std::stringstream buffer;
    
public:
    BufferedConnection();
    BufferedConnection(int client_sock);
    BufferedConnection(BufferedConnection&&);
    ~BufferedConnection();
    
    bool is_closed();
    void close();
    void write(std::string body);
    std::string read_until(std::string sep);
};

#endif // CONNECTION_H
