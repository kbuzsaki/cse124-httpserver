#include <iostream>
#include <chrono>
#include <poll.h>
#include <stdexcept>
#include "async_event_loop.h"
#include "util.h"

using std::chrono::system_clock;
using std::shared_ptr;
using std::runtime_error;
using std::vector;

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

void AsyncEventLoop::process_pollable(shared_ptr<Pollable> pollable, short revents) {
    shared_ptr<Pollable> next_pollable;
    try {
        next_pollable = pollable->notify(revents);
        if (!pollable->is_done()) {
            register_pollable(pollable);
        }
    } catch (runtime_error& e) {
        std::cerr << "warning: uncaught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "warning: unknown uncaught exception!" << std::endl;
    }

    if (next_pollable != NULL) {
        register_pollable(next_pollable);
    }
}

void AsyncEventLoop::loop() {
    while (!pollables.empty()) {
        vector<shared_ptr<Pollable>> cur_pollables = pollables;
        vector<shared_ptr<Pollable>> skipped;
        pollables.clear();

        size_t pollables_size = cur_pollables.size();
        pollfd* pollfds = new pollfd[pollables_size];

        for (size_t i = 0; i < pollables_size; i++) {
            pollfds[i] = pollfd_from_pollable(cur_pollables[i]);
        }

        int ret = poll(pollfds, pollables_size, DEFAULT_TIMEOUT);
        if (ret < 0) {
            throw runtime_error(errno_message("poll() failed: "));
        }

        for (size_t i = 0; i < pollables_size; i++) {
            short revents = pollfds[i].revents;

            if (revents != 0) {
                process_pollable(cur_pollables[i], revents);
            } else {
                skipped.push_back(cur_pollables[i]);
            }
        }

        delete[] pollfds;

        // This weird copying back and forth below serves two purposes:
        // 1. it prunes all of the skipped file descriptors that are now past their deadlines.
        // 2. it reorders the connections such that skipped descriptors are at the front of the list.
        //   I found anecdotally that this helped to avoid starvation of descriptors at the end of the vector
        //   I could have done this processing in the processing loop above using linked lists to perform the
        //   same effect with fewer loops and still O(n) time, but I found that doing so made the code much
        //   more convoluted so I decided to separate the behavior out. The resulting algorithm is still
        //   O(n) + O(n) = O(n) and I haven't noticed any performance problems stemming from the additional
        //   copies (it is just copying pointers), so I decided that it was sufficient.
        vector<shared_ptr<Pollable>> new_pollables;

        // check timeouts for every fd that was skipped
        system_clock::time_point now = system_clock::now();
        for (size_t i = 0; i < skipped.size(); i++) {
            if (!skipped[i]->past_deadline(now)) {
                new_pollables.push_back(skipped[i]);
            }
        }

        for (size_t i = 0; i < pollables.size(); i++) {
            new_pollables.push_back(pollables[i]);
        }
        pollables = new_pollables;
    }
}
