#include <iostream>
#include <poll.h>
#include <stdexcept>
#include "async_event_loop.h"
#include "util.h"

using std::shared_ptr;
using std::runtime_error;

#define DEFAULT_TIMEOUT (1 * 1000)


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

        std::cerr << "poll() returned: " << ret << std::endl;

        // TODO: prune pollables?
        for (size_t i = 0; i < pollables_size; i++) {
            short revents = pollfds[i].revents;

            std::cerr << "revents for fd " << pollfds[i].fd << ": " << std::hex << revents << std::endl;

            if (revents != 0) {
                shared_ptr<Pollable> pollable = pollables[i]->notify(revents);

                if (pollable != NULL) {
                    register_pollable(pollable);
                }
            }
        }


        // TODO: exception safety?
        delete[] pollfds;
    }
}
