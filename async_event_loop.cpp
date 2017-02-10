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

int AsyncEventLoop::process_pollable(int index, short revents) {
    shared_ptr<Pollable> pollable;
    try {
        pollable = pollables[index]->notify(revents);
        if (pollables[index]->is_done()) {
            pollables.erase(pollables.begin() + index);
        } else {
            index++;
        }
    } catch (...) {
        // the pollable errored, so just give up on it
        pollables.erase(pollables.begin() + index);
    }

    if (pollable != NULL) {
        register_pollable(pollable);
    }

    return index;
}

void AsyncEventLoop::loop() {
    while (!pollables.empty()) {
        nfds_t pollables_size = (nfds_t) pollables.size();
        pollfd* pollfds = new pollfd[pollables_size];

        for (size_t i = 0; i < pollables_size; i++) {
            pollfds[i] = pollfd_from_pollable(pollables[i]);
        }

        int ret = poll(pollfds, pollables_size, DEFAULT_TIMEOUT);
        if (ret < 0) {
            throw runtime_error(errno_message("poll() failed: "));
        }

        size_t j = 0;
        for (size_t i = 0; i < pollables_size; i++) {
            short revents = pollfds[i].revents;

            if (revents != 0) {
                j = process_pollable(j, revents);
            } else {
                // TODO: check timeout
                j++;
            }
        }

        // TODO: exception safety?
        delete[] pollfds;
    }
}
