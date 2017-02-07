#include "async_listener.h"

using namespace std;

int main() {

    shared_ptr<AsyncSocketListener> listener = make_shared<AsyncSocketListener>(6060);
    listener->listen();

    AsyncEventLoop loop;

    loop.register_pollable(listener);

    loop.loop();

    return 0;
}
