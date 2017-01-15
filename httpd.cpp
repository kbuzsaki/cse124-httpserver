#include <iostream>
#include "httpd.h"
#include "connection.h"

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
        BufferedConnection connection = listener.accept();

        string received = "";
        do {
            received = connection.read_until("\n");
            cout << received;
            connection.write(received);
        } while(received.size() > 0);
        
        cout << "client finished sending" << endl;
    }
}
