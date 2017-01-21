#ifndef MOCKS_H
#define MOCKS_H

#include <memory>
#include <unordered_map>
#include "connection.h"
#include "http.h"
#include "file_repository.h"
#include "listener.h"
#include "server.h"


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


class MockListener : public Listener {
    std::vector<std::shared_ptr<Connection>> connections;

public:
    MockListener(std::vector<std::shared_ptr<Connection>> connections);
    MockListener(std::vector<std::shared_ptr<MockConnection>> connections);
    virtual ~MockListener();

    virtual void listen();
    virtual std::shared_ptr<Connection> accept();
};


class MockHttpHandler : public HttpHandler {
    HttpResponse response_payload;
    std::vector<HttpRequest> request_copies;

public:
    MockHttpHandler(const HttpResponse& response);

    virtual HttpResponse handle_request(const HttpRequest&);

    const std::vector<HttpRequest>& requests();
};


class MockFile : public File {
    const bool world_readable_payload;
    const std::string contents_payload;
    const std::chrono::system_clock::time_point last_modified_payload;

public:
    MockFile(const bool& world_readable, const std::string& contents, const std::chrono::system_clock::time_point& last_modified);

    virtual bool world_readable();
    virtual std::string contents();
    virtual std::chrono::system_clock::time_point last_modified();
};


class MockFileRepository : public FileRepository {
    const std::unordered_map<std::string, std::shared_ptr<File>> mock_files;

public:
    MockFileRepository(std::unordered_map<std::string, std::shared_ptr<File>> mock_files);

    virtual std::shared_ptr<File> get_file(std::string path);
};

#endif //MOCKS_H
