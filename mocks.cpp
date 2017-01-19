#include <algorithm>
#include <iostream>
#include "util.h"
#include "mocks.h"

using std::shared_ptr;
using std::string;
using std::vector;

#define DEFAULT_READ_SIZE (100)
#define BUFFER_SIZE (5000)


MockConnection::MockConnection(string payload) : read_payload(payload), write_payload(), read_size(DEFAULT_READ_SIZE), closed(false) {}

MockConnection::MockConnection(string payload, int read_size) : read_payload(payload), write_payload(), read_size(read_size), closed(false) {}

MockConnection::~MockConnection() {}

// TODO: make closed make these fail
std::string MockConnection::read() {
    char buf[BUFFER_SIZE];

    read_payload.read(buf, read_size);
    buf[read_payload.gcount()] = '\0';

    return string(buf);
}

void MockConnection::write(string s) {
    this->write_payload << s;
}

void MockConnection::close() {
    closed = true;
}

bool MockConnection::is_closed() {
    return closed;
}

string MockConnection::written() {
    return write_payload.str();
}


MockListener::MockListener(vector<shared_ptr<Connection>> connections) : connections(connections) {
    std::reverse(begin(this->connections), end(this->connections));
}

MockListener::MockListener(std::vector<std::shared_ptr<MockConnection>> mock_connections) {
    for (size_t i = 0; i < mock_connections.size(); i++) {
        this->connections.push_back(mock_connections[i]);
    }
    std::reverse(begin(this->connections), end(this->connections));
}

MockListener::~MockListener() {}

void MockListener::listen() { /* NOOP */ }

shared_ptr<Connection> MockListener::accept() {
    if (connections.empty()) {
        throw ListenerError("no more connections from mock listener");
    }

    shared_ptr<Connection> conn = connections.at(connections.size() - 1);
    connections.pop_back();
    return conn;
}


MockHttpHandler::MockHttpHandler(const HttpResponse &response) : response_payload(response), request_copies() {}

HttpResponse MockHttpHandler::handle_request(const HttpRequest& request) {
    request_copies.push_back(request);
    return response_payload;
}

const vector<const HttpRequest>& MockHttpHandler::requests() {
    return request_copies;
}
