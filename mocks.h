#ifndef MOCKS_H
#define MOCKS_H

#include <memory>
#include <unordered_map>
#include "connection.h"
#include "dns_client.h"
#include "http.h"
#include "file_repository.h"
#include "listener.h"
#include "request_filters.h"
#include "server.h"


/*
 * MockConnection implements Connection with in memory string buffers to assist in testing.
 * `read` is implemented by reading in specified chunk sizes from a preset buffer until
 * the buffer is empty, after which additional calls will throw ConnectionClosed
 * `write` is implemented by appending to an internal buffer.
 * The written bytes can be inspected for verification using the `written` method.
 */
class MockConnection : public Connection {
    std::stringstream read_payload;
    std::stringstream write_payload;
    int read_size;
    bool closed;
    struct in_addr mock_remote_ip;

public:
    MockConnection(std::string payload, struct in_addr mock_remote_ip={0});
    MockConnection(std::string payload, int read_size, struct in_addr mock_remote_ip={0});
    virtual ~MockConnection();

    virtual std::string read();
    virtual void write(std::string);
    virtual void close();
    virtual bool is_closed();

    virtual struct in_addr remote_ip();

    std::string written();
};


/*
 * MockListener implements Listener by popping connections from an internal vector
 * of preset Connections, and then throwing ListenerError when empty.
 */
class MockListener : public Listener {
    std::vector<std::shared_ptr<Connection>> connections;

public:
    MockListener(std::vector<std::shared_ptr<Connection>> connections);
    MockListener(std::vector<std::shared_ptr<MockConnection>> connections);
    virtual ~MockListener();

    virtual void listen();
    virtual std::shared_ptr<Connection> accept();
};


/*
 * MockHttpRequestHandler implements HttpRequestHandler with preset HttpResponse and
 * an in memory buffer of received requests.
 * Calls to `handle_request` append the request to the internal buffer of requests and
 * return the given response.
 * Received requests can be inspected for verification using the `requests` method.
 */
class MockHttpRequestHandler : public HttpRequestHandler {
    HttpResponse response_payload;
    std::vector<HttpRequest> request_copies;

public:
    MockHttpRequestHandler(const HttpResponse& response);

    virtual HttpResponse handle_request(const HttpRequest&);

    const std::vector<HttpRequest>& requests();
};


/*
 * MockFile implements File by returning the given preset values when accessed.
 */
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


/*
 * MockFileRepository implements FileRepository with an in memory map from paths to Files.
 * If a path is not present in its map, it returns NULL.
 */
class MockFileRepository : public FileRepository {
    const std::unordered_map<std::string, std::shared_ptr<File>> mock_files;

public:
    MockFileRepository(std::unordered_map<std::string, std::shared_ptr<File>> mock_files);

    virtual std::shared_ptr<File> get_file(std::string path);
};


/*
 * MockDnsClient implements DnsClient with an in memory map from domain strings to vectors
 * of in_addr structs. If the domain string is not present in the map, it returns empty vector.
 */
class MockDnsClient : public DnsClient {
    const std::unordered_map<std::string, std::vector<struct in_addr>> mock_results;

public:
    MockDnsClient(std::unordered_map<std::string, std::vector<struct in_addr>> mock_results);

    virtual std::vector<struct in_addr> lookup(std::string domain);
};


/*
 * MockRequestFilter implements RequestFilter with an in memory list of request, allow/deny pairs.
 * If a request is not present in its list, defaults to true.
 * The implementation uses a vector of pairs rather than a map because HttpRequest is a user defined
 * type and I didn't want to go to the trouble of making HttpRequest hashable when this method is
 * simpler and there's no apparent performance cost to the test suite.
 */
class MockRequestFilter : public RequestFilter {
    const std::vector<std::pair<HttpRequest, bool>> mock_results;

public:
    MockRequestFilter(std::vector<std::pair<HttpRequest, bool>> mock_results);

    virtual bool allow_request(const HttpRequest& request);
};

#endif //MOCKS_H
