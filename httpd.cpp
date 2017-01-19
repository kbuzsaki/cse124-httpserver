#include <iostream>
#include "httpd.h"
#include "connection.h"
#include "http.h"
#include "server.h"
#include "file_repository.h"
#include "handlers.h"

using std::cerr;
using std::cout;
using std::make_shared;
using std::string;
using std::shared_ptr;
using std::endl;

#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

class LoggingHttpHandler : public HttpHandler {

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

void start_httpd(unsigned short port, string doc_root) {
    cerr << "Starting server (port: " << port << ", doc_root: " << doc_root << ")" << endl;

    shared_ptr<FileRepository> repository = make_shared<DirectoryFileRepository>(doc_root);
    HttpServer server(HttpListener(make_shared<SocketListener>(port)), make_shared<FileServingHttpHandler>(repository));
    server.serve();
}
