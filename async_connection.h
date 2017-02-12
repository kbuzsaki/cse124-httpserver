#ifndef ASYNC_CONNECTION_H
#define ASYNC_CONNECTION_H

#include <functional>
#include <netinet/in.h>
#include <memory>
#include <sstream>
#include "async_event_loop.h"

/*
 * AutoClosingSocket wraps a socket and calls close on it in its destructor.
 * The socket is a public member and there are no other convenience functions provided.
 */
struct AutoClosingSocket {
    int client_sock;

    AutoClosingSocket(int sock);
    ~AutoClosingSocket();
};

/*
 * AsyncSocketConnection represents a nonblocking bydirectional bytestream.
 * `read` takes a callback to be invoked when data is ready and returns a Pollable
 *        to be enqueued in the event loop
 * `write` takes a string to write and a callback to invoke when the write is complete
 *        and returns a Pollable to be enqueued in the event loop
 */
class AsyncSocketConnection {
    std::shared_ptr<AutoClosingSocket> conn;
    struct in_addr client_remote_ip;

public:
    AsyncSocketConnection(int client_sock, struct in_addr client_remote_ip);
    AsyncSocketConnection(AsyncSocketConnection&&);

    struct in_addr get_remote_ip();

    virtual std::shared_ptr<Pollable> read(Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> write(std::string, Callback<>::F callback);
};


/*
 * AsyncBufferedConnection provides a slightly higher level interface than AsyncSocketConnection,
 * allowing read_until operations similar to BufferedConnection in connection.h
 * As above, `read_until` and `write` take callbacks to be invoked when their operation is complete
 * and return pollables to be enqueued in the event loop.
 */
class AsyncBufferedConnection {
    std::shared_ptr<AsyncSocketConnection> conn;
    std::shared_ptr<std::stringstream> buffer;

public:
    AsyncBufferedConnection(std::shared_ptr<AsyncSocketConnection> conn);

    struct in_addr get_remote_ip();

    virtual std::shared_ptr<Pollable> read_until(std::string sep, Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> write(std::string, Callback<>::F callback);
};


#endif //ASYNC_CONNECTION_H
