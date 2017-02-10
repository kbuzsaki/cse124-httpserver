#include <iostream>
#include <thread>
#include <stdexcept>
#include "connection_handlers.h"

using std::cerr;
using std::endl;
using std::exception;
using std::thread;
using std::shared_ptr;
using std::make_shared;


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

            if (get_header(request.headers, "Connection").value == "close") {
                return;
            }
        }
    } catch (HttpRequestParseError&) {
        conn.write_response(bad_request_response());
    } catch (ConnectionClosed&) {
        return;
    } catch (exception& e) {
        cerr << "Encountered unexpected exception: " << e.what() << endl;
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


void handle_work_queue(shared_ptr<SynchronizedQueue<shared_ptr<HttpConnection>>> work_queue, shared_ptr<HttpRequestHandler> handler) {
    while (true) {
        shared_ptr<HttpConnection> conn_ptr = work_queue->pop();

        try {
            handle_connection(handler, std::move(*conn_ptr));
        } catch (...) {
            cerr << "ERROR: exception bubbled up to top level threadpool function!" << endl;
        }
    }
}

ThreadPoolHttpConnectionHandler::ThreadPoolHttpConnectionHandler(shared_ptr<HttpRequestHandler> handler, int size)
    : handler(handler), thread_pool(), work_queue() {
    work_queue = make_shared<SynchronizedQueue<shared_ptr<HttpConnection>>>();

    for (int i = 0; i < size; i++) {
        thread_pool.push_back(thread(handle_work_queue, work_queue, handler));
    }
}

void ThreadPoolHttpConnectionHandler::handle_connection(HttpConnection&& conn) {
    work_queue->push(make_shared<HttpConnection>(std::move(conn)));
}
