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

void AsyncEventLoop::process_pollable(shared_ptr<Pollable> pollable, short revents) {
    shared_ptr<Pollable> next_pollable;
    try {
        next_pollable = pollable->notify(revents);
        if (!pollable->is_done()) {
            register_pollable(pollable);
        }
    } catch (...) {
    }

    if (next_pollable != NULL) {
        register_pollable(next_pollable);
    }
}

void AsyncEventLoop::loop() {
    while (!pollables.empty()) {
        vector<shared_ptr<Pollable>> cur_pollables = pollables;
        pollables.clear();
        vector<shared_ptr<Pollable>> skipped;

        size_t pollables_size = cur_pollables.size();
        pollfd* pollfds = new pollfd[pollables_size];

        for (size_t i = 0; i < pollables_size; i++) {
            pollfds[i] = pollfd_from_pollable(pollables[i]);
        }

        int ret = poll(pollfds, pollables_size, DEFAULT_TIMEOUT);
        if (ret < 0) {
            throw runtime_error(errno_message("poll() failed: "));
        }

        for (size_t i = 0; i < pollables_size; i++) {
            short revents = pollfds[i].revents;

            if (revents != 0) {
                //std::cerr << "have revents " << revents << " for fd " << pollfds[i].fd << std::endl;
                process_pollable(cur_pollables[i], revents);
            } else {
                // TODO: check timeout
                //std::cerr << "no revents for fd " << pollfds[i].fd << std::endl;
                skipped.push_back(cur_pollables[i]);
            }
        }

        // TODO: exception safety?
        delete[] pollfds;

        vector<shared_ptr<Pollable>> temp = skipped;
        for (size_t i = 0; i < pollables.size(); i++) {
            temp.push_back(pollables[i]);
        }
        pollables = temp;
    }
}
