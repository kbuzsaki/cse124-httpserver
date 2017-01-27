#ifndef HTTPD_H
#define HTTPD_H

#include <string>

typedef int ThreadModel;

const ThreadModel NO_THREADS = -1;
const ThreadModel NO_POOL = 0;

void start_httpd(unsigned short port, std::string doc_root, ThreadModel thread_model);

#endif // HTTPD_H
