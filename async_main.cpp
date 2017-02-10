#include <iostream>
#include "async_listener.h"
#include "async_http_server.h"
#include "async_request_handlers.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "usage" << endl;
        return 1;
    }

    long int port = strtol(argv[1], NULL, 10);
    string doc_root = string(argv[2]);

    shared_ptr<AsyncSocketListener> listener = make_shared<AsyncSocketListener>(port);

    shared_ptr<AsyncFileRepository> repository = make_shared<DirectoryAsyncFileRepository>(doc_root);
    AsyncHttpServer server(listener, make_shared<FileServingAsyncHttpRequestHandler>(repository));

    server.serve();

    return 0;
}
