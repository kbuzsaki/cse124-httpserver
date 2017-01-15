#include <iostream>
#include "httpd.h"
#include "connection.h"
#include "http.h"

using std::cerr;
using std::cout;
using std::string;
using std::endl;

#define QUEUE_SIZE (100)
#define BUFFER_SIZE (2000)

void start_httpd(unsigned short port, string doc_root) {
    cerr << "Starting server (port: " << port << ", doc_root: " << doc_root << ")" << endl;

    SocketListener listener(port);
    listener.listen();

    while (true) {
        HttpConnection connection(listener.accept());

        HttpRequest request = connection.read_request();
        cout << "method: '" << request.method << "'" << endl;
        cout << "uri: '" << request.uri << "'" << endl;
        cout << "version: '" << request.version << "'" << endl;
        cout << "headers:" << endl;
        for (size_t i = 0; i < request.headers.size(); i++) {
            cout << request.headers[i].key << ": " << request.headers[i].value << endl;
        }
        cout << endl;

        if (request.uri == "/shutdown") {
            connection.write_response(internal_server_error_response());
            return;
        } else {
            connection.write_response(not_found_response());
        }
        
        cout << "client finished sending" << endl << endl;
    }
}
