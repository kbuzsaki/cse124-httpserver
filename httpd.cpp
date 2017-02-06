#include <iostream>
#include "httpd.h"
#include "connection.h"
#include "connection_handlers.h"
#include "http.h"
#include "server.h"
#include "file_repository.h"
#include "request_handlers.h"

using std::cerr;
using std::cout;
using std::invalid_argument;
using std::make_shared;
using std::string;
using std::shared_ptr;
using std::endl;

#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

class LoggingHttpRequestHandler : public HttpRequestHandler {

public:
    HttpResponse handle_request(const HttpRequest& request) {
        cout << "method: '" << request.method << "'" << endl;
        cout << "uri: '" << request.uri << "'" << endl;
        cout << "version: '" << request.version << "'" << endl;
        cout << "headers:" << endl;
        for (size_t i = 0; i < request.headers.size(); i++) {
            cout << request.headers[i].key << ": " << request.headers[i].value << endl;
        }
        cout << "body: '" << request.body << "'" << endl;
        cout << endl;

        return not_found_response();
    }
};


shared_ptr<HttpRequestHandler> wrap_htaccess_middleware(shared_ptr<FileRepository> repository, shared_ptr<HttpRequestHandler> handler) {
    shared_ptr<DnsClient> dns_client = make_shared<NetworkDnsClient>();
    shared_ptr<RequestFilter> htaccess_filter = make_shared<HtAccessRequestFilter>(repository, dns_client);
    return make_shared<RequestFilterMiddleware>(htaccess_filter, handler);
}

void start_httpd(unsigned short port, string doc_root, ThreadModel thread_model) {
    cerr << "Starting server (port: " << port << ", doc_root: " << doc_root << ")" << endl;

    shared_ptr<FileRepository> repository = make_shared<DirectoryFileRepository>(doc_root);
    shared_ptr<HttpRequestHandler> file_serving_handler = make_shared<FileServingHttpHandler>(repository);

    shared_ptr<HttpRequestHandler> request_handler = wrap_htaccess_middleware(repository, file_serving_handler);

    shared_ptr<HttpConnectionHandler> connection_handler;
    if (thread_model == NO_THREADS) {
        connection_handler = make_shared<BlockingHttpConnectionHandler>(request_handler);
    } else if (thread_model == NO_POOL) {
        connection_handler = make_shared<ThreadSpawningHttpConnectionHandler>(request_handler);
    } else {
        connection_handler = make_shared<ThreadPoolHttpConnectionHandler>(request_handler, (int)thread_model);
    }

    HttpServer server(HttpListener(make_shared<SocketListener>(port)), connection_handler);
    server.serve();
}
