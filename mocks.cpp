#include <algorithm>
#include <iostream>
#include "util.h"
#include "mocks.h"

using std::chrono::system_clock;
using std::shared_ptr;
using std::string;
using std::vector;

#define DEFAULT_READ_SIZE (100)
#define BUFFER_SIZE (5000)


MockConnection::MockConnection(string payload, struct in_addr mock_remote_ip)
        : read_payload(payload), write_payload(), read_size(DEFAULT_READ_SIZE), closed(false), mock_remote_ip(mock_remote_ip) {}

MockConnection::MockConnection(string payload, int read_size, struct in_addr mock_remote_ip)
        : read_payload(payload), write_payload(), read_size(read_size), closed(false), mock_remote_ip(mock_remote_ip) {}

MockConnection::~MockConnection() {}

// TODO: make closed make these fail
std::string MockConnection::read() {
    char buf[BUFFER_SIZE];

    read_payload.read(buf, read_size);
    ssize_t received = read_payload.gcount();
    if (received == 0) {
        throw ConnectionClosed();
    }

    buf[received] = '\0';
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

struct in_addr MockConnection::remote_ip() {
    return mock_remote_ip;
}

string MockConnection::written() {
    return write_payload.str();
}


MockListener::MockListener(vector<shared_ptr<Connection>> connections) : connections(connections) {
    std::reverse(this->connections.begin(), this->connections.end());
}

MockListener::MockListener(std::vector<std::shared_ptr<MockConnection>> mock_connections) {
    for (size_t i = 0; i < mock_connections.size(); i++) {
        this->connections.push_back(mock_connections[i]);
    }
    std::reverse(this->connections.begin(), this->connections.end());
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


MockHttpRequestHandler::MockHttpRequestHandler(const HttpResponse &response) : response_payload(response), request_copies() {}

HttpResponse MockHttpRequestHandler::handle_request(const HttpRequest& request) {
    request_copies.push_back(request);
    return response_payload;
}

const vector<HttpRequest>& MockHttpRequestHandler::requests() {
    return request_copies;
}


MockFile::MockFile(const bool& world_readable, const string& contents, const system_clock::time_point& last_modified)
        : world_readable_payload(world_readable), contents_payload(contents), last_modified_payload(last_modified) {}

bool MockFile::world_readable() {
    return world_readable_payload;
}

std::string MockFile::contents() {
    return contents_payload;
}

system_clock::time_point MockFile::last_modified() {
    return last_modified_payload;
}


MockFileRepository::MockFileRepository(std::unordered_map<std::string, std::shared_ptr<File>> mock_files) : mock_files(mock_files) {}

std::shared_ptr<File> MockFileRepository::get_file(std::string path) {
    if (mock_files.count(path) == 0) {
        return shared_ptr<File>();
    }

    return mock_files.at(path);
}


MockDnsClient::MockDnsClient(std::unordered_map<std::string, std::vector<struct in_addr>> mock_results) : mock_results(mock_results) {}

std::vector<struct in_addr> MockDnsClient::lookup(std::string domain) {
    if (mock_results.count(domain) == 0) {
        return vector<struct in_addr>();
    }

    return mock_results.at(domain);
}
