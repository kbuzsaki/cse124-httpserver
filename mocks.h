#ifndef MOCKS_H
#define MOCKS_H

#include "connection.h"
#include "handler.h"
#include "http.h"


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

#endif //MOCKS_H
