#ifndef HTTPD_H
#define HTTPD_H

#include <string>

/*
 * ThreadModel represents the different possible threading models used by the server.
 * A positive value indicates a thread pool with that many threads in the pool.
 * The constants declared below represent different models as follows:
 * NO_POOL: a thread-per-connection model
 * NO_THREADS: a blocking model that handles requests on the main thread
 * ASYNC_EVENT_LOOP: an non-blocking model that handles requests on the
 *                   main thread using asynchronous IO.
 */
typedef int ThreadModel;

const ThreadModel ASYNC_EVENT_LOOP = -2;
const ThreadModel NO_THREADS = -1;
const ThreadModel NO_POOL = 0;

void start_httpd(unsigned short port, std::string doc_root, ThreadModel thread_model);

#endif // HTTPD_H
