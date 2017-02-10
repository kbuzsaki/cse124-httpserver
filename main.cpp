#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdexcept>
#include <vector>
#include "httpd.h"

using namespace std;

const ThreadModel DEFAULT_THREAD_MODEL = NO_POOL;
const vector<string> THREAD_MODELS = vector<string>{"nothread", "nopool", "pool", "async"};

void usage(char* argv0) {
    cerr << "Usage: " << argv0 << " listen_port docroot_dir [nothread | nopool | pool size | async]" << endl;
}

uint16_t parse_port(char* port_str) {
    long int port = strtol(port_str, NULL, 10);

    if (errno == EINVAL || errno == ERANGE) {
        throw invalid_argument(string("Invalid port: ") + port_str);
    }

    if (port <= 0 || port > USHRT_MAX) {
        throw invalid_argument("Port out of range: " + to_string(port));
    }

    return (uint16_t) port;
}

ThreadModel parse_thread_model(int argc, char** argv) {
    if (argc == 0) {
        return DEFAULT_THREAD_MODEL;
    }

    string thread_model = argv[0];
    if (find(THREAD_MODELS.begin(), THREAD_MODELS.end(), thread_model) == THREAD_MODELS.end()) {
        throw invalid_argument("Invalid thread model: " + thread_model);
    }

    if (thread_model == "nothread") {
        return NO_THREADS;
    } else if (thread_model == "nopool") {
        return NO_POOL;
    } else if (thread_model == "async") {
        return ASYNC_EVENT_LOOP;
    } else {
        if (argc >= 2) {
            return ThreadModel(strtol(argv[1], NULL, 10));
        } else {
            return ThreadModel(4);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        usage(argv[0]);
        return 1;
    }

    try {
        uint16_t port = parse_port(argv[1]);
        string doc_root = argv[2];
        ThreadModel thread_model = parse_thread_model(argc - 3, argv + 3);

        start_httpd(port, doc_root, thread_model);
    } catch (invalid_argument& e) {
        cerr << e.what() << endl;
        usage(argv[0]);
        return 1;
    }

    return 0;
}
