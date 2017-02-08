#include <iostream>
#include "async_listener.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "usage" << endl;
        return 1;
    }

    long int port = strtol(argv[1], NULL, 10);
    string doc_root = string(argv[2]);

    shared_ptr<AsyncSocketListener> listener = make_shared<AsyncSocketListener>(port);
    listener->listen();

    AsyncEventLoop loop;
    loop.register_pollable(listener);
    loop.loop();

    return 0;
}
