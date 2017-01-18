
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <vector>
#include "connection.h"
#include "handler.h"
#include "http.h"

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i == 0) {
            os << "'" << v[i] << "'";
        } else {
            os << ", '" << v[i] << "'";
        }
    }
    return os << "]";
}

std::vector<std::string> split(std::string s, std::string sep);

std::vector<std::string> split_n(std::string s, std::string sep, int n_splits);

std::string pop_n_sstream(std::stringstream& buffer, size_t n, size_t discard);

size_t sstream_size(std::stringstream& buffer);

class MockListener : public Listener {
    std::vector<Connection*> connections;

public:
    MockListener(std::vector<Connection*> connections);
    virtual ~MockListener();

    virtual void listen();
    virtual Connection* accept();
};

class MockConnection : public Connection {
    std::stringstream read_payload;
    std::stringstream write_payload;
    int read_size;
    bool closed;

public:
    MockConnection(std::string payload);
    MockConnection(std::string payload, int read_size);
    virtual ~MockConnection();

    virtual std::string read();
    virtual void write(std::string);
    virtual void close();
    virtual bool is_closed();

    std::string written();
};

class MockHttpHandler : public HttpHandler {
    HttpResponse response_payload;
    HttpRequest request_copy;

public:
    MockHttpHandler(const HttpResponse& response);

    virtual HttpResponse handle_request(const HttpRequest&);

    HttpRequest request();
};

#endif //UTIL_H
