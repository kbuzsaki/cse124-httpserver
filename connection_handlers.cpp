#include <thread>
#include <stdexcept>
#include "connection_handlers.h"

using std::exception;
using std::thread;
using std::shared_ptr;


void handle_connection(shared_ptr<HttpRequestHandler> handler, HttpConnection&& conn) {
    try {
        while (true) {
            HttpRequest request = conn.read_request();
            if (!has_header(request.headers, "Host")) {
                conn.write_response(bad_request_response());
                return;
            } else {
                HttpResponse response = handler->handle_request(request);
                conn.write_response(response);
            }
        }
    } catch (HttpRequestParseError&) {
        conn.write_response(bad_request_response());
    } catch (ConnectionClosed&) {
        return;
    } catch (exception&) {
        conn.write_response(internal_server_error_response());
    }
}


BlockingHttpConnectionHandler::BlockingHttpConnectionHandler(shared_ptr<HttpRequestHandler> handler) : handler(handler) {}

void BlockingHttpConnectionHandler::handle_connection(HttpConnection&& conn) {
    ::handle_connection(handler, std::move(conn));
}


ThreadSpawningHttpConnectionHandler::ThreadSpawningHttpConnectionHandler(shared_ptr<HttpRequestHandler> handler) : handler(handler) {}

void ThreadSpawningHttpConnectionHandler::handle_connection(HttpConnection&& conn) {
    thread th(::handle_connection, handler, std::move(conn));
    th.detach();
}
