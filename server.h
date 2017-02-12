#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include "http.h"
#include "listener.h"


/*
 * HttpConnection wraps a bytestream Connection object and provides higher level
 * operations for processing http requests and responess.
 * The `read_request` method reads and parses an HttpRequest from the connection.
 * If the request is malformed it throws HttpRequestParseError. If the underlying
 * connection closes, it throws ConnectionClosed.
 * The `write_response` method serializes and sends an HttpResponse. It throws
 * ConnectionClosed if the underlying connection closes.
 */
class HttpConnection {
    BufferedConnection conn;

    HttpFrame read_frame();
    void write_frame(HttpFrame frame);

public:
    HttpConnection(std::shared_ptr<Connection> conn);
    HttpConnection(HttpConnection&&);

    HttpRequest read_request();
    void write_response(HttpResponse);
};


/*
 * HttpListener wraps a Listener object and returns accepted connections prewrapped
 * as HttpConnection objects.
 */
class HttpListener {
    std::shared_ptr<Listener> listener;

public:
    HttpListener(std::shared_ptr<Listener>);
    HttpListener(HttpListener&&);

    void listen();
    HttpConnection accept();
};


/*
 * HttpRequestHandler is an abstract class that represents the minimal interface for
 * handling HttpRequests received by the HttpServer.
 * It is implemented by FileServingHttpHandler and RequestFilterMiddleware in request_handlers.h
 * and by MockHttpRequestHandler in mocks.h
 */
class HttpRequestHandler {
public:
    virtual ~HttpRequestHandler() {};

    virtual HttpResponse handle_request(const HttpRequest&) = 0;
};


/*
 * HttpConnectionHandler is an abstract class that represents different strategies
 * for handling incoming HttpConnections. It is implemented by BlockingHttpConnectionHandler,
 * ThreadSpawningHttpConnectionHandler, and ThreadPoolHttpConnectionHandler in
 * connection_handlers.h
 */
class HttpConnectionHandler {
public:
    virtual ~HttpConnectionHandler() {};

    virtual void handle_connection(HttpConnection&&) = 0;
};


/*
 * HttpServer takes an HttpListener and HttpConnectionHandler and performs the mechanical
 * work of reading incoming HttpConnections from the HttpListener and passing
 * them off to the HttpConnectionHandler.
 */
class HttpServer {
    HttpListener listener;
    std::shared_ptr<HttpConnectionHandler> handler;

public:
    HttpServer(HttpListener&&, std::shared_ptr<HttpConnectionHandler>);

    void serve();
};

#endif //SERVER_H
