#include <iostream>
#include <poll.h>
#include <stdexcept>
#include "async_event_loop.h"
#include "util.h"

using std::shared_ptr;
using std::runtime_error;
using std::vector;

#define DEFAULT_TIMEOUT (10 * 1000)


void AsyncEventLoop::register_pollable(shared_ptr<Pollable> pollable) {
    pollables.push_back(pollable);
}

pollfd pollfd_from_pollable(shared_ptr<Pollable> pollable) {
    return pollfd{
        pollable->get_fd(),
        pollable->get_events(),
        0
    };
}

vector<shared_ptr<Pollable>>::iterator AsyncEventLoop::process_pollable(vector<shared_ptr<Pollable>>::iterator iter, short revents) {
    shared_ptr<Pollable> pollable;
    try {
        pollable = (*iter)->notify(revents);
        if ((*iter)->is_done()) {
            iter = pollables.erase(iter);
        } else {
            iter++;
        }
    } catch (...) {
        // the pollable errored, so just give up on it
        iter = pollables.erase(iter);
    }

    if (pollable != NULL) {
        register_pollable(pollable);
    }

    return iter;
}

void AsyncEventLoop::loop() {
    while (!pollables.empty()) {
        nfds_t pollables_size = (nfds_t) pollables.size();
        pollfd* pollfds = new pollfd[pollables_size];

        size_t i = 0;
        for (auto iter = pollables.begin(); iter != pollables.end() && i < pollables_size; i++, iter++) {
            pollfds[i] = pollfd_from_pollable(*iter);
        }

        int ret = poll(pollfds, pollables_size, DEFAULT_TIMEOUT);
        if (ret < 0) {
            throw runtime_error(errno_message("poll() failed: "));
        }

        i = 0;
        for (auto iter = pollables.begin(); i < pollables_size && iter != pollables.end(); i++) {
            short revents = pollfds[i].revents;

            if (revents != 0) {
                iter = process_pollable(iter, revents);
            } else {
                // TODO: check timeout
                iter++;
            }
        }

        // TODO: exception safety?
        delete[] pollfds;
    }
}
