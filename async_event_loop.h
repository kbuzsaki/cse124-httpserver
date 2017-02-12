#ifndef ASYNC_EVENT_LOOP_H
#define ASYNC_EVENT_LOOP_H

#include <chrono>
#include <functional>
#include <memory>
#include <vector>


/*
 * Pollable is an abstract class representing a non-blocking IO operation that can be
 * polled with the poll() system call.
 * The async event loop manages a vector of pollables and will notify them when their
 * file descriptor is ready for their requested operation and of periodic time points
 * so that they can decide whether or not they are passed their deadline.
 *
 * Implementations of pollable should typically contain a Callback<...>::F to pass their
 * results to once they are finished.
 */
class Pollable {
public:
    virtual ~Pollable() {};

    // get_fd returns the file descriptor for poll() to listen on
    virtual int get_fd() = 0;

    // get_events returns the events that poll() should check for
    virtual short get_events() = 0;

    // is_done returns true if this pollable has completed and should be removed from the queue
    virtual bool is_done() = 0;

    // past_deadline returns true if this pollable operation has timed out and the pollable should be removed
    virtual bool past_deadline(std::chrono::system_clock::time_point) = 0;

    /**
     * notify is used to notify the Pollable that one of its events has occurred
     * @param revents  the events from poll() that occurred
     * @return another Pollable to listen to, or null if nothing more should be listened to
     */
    virtual std::shared_ptr<Pollable> notify(short revents) = 0;
};


// Convenience typedef to enable `Callback<string>::F` instead of `std::function<std::shared_ptr<Pollable> (string)>`
/*
 * Callback is a convenience typedef to enable `Callback<string>::F` instead of `std::function<std::shared_ptr<Pollable> (string)>`
 * A Callback represents a unit of work to be invoked at some time in the future once its inputs are ready.
 * It optionally a Pollable to be added to the event loop once its unit of work is complete.
 */
template<class ...Args>
struct Callback {
    typedef std::function<std::shared_ptr<Pollable> (Args...)> F;

    /*
     * empty() is a convenience function that creates a Callback that returns NULL.
     */
    static F empty() {
        return [](Args...) -> std::shared_ptr<Pollable> {
            return std::shared_ptr<Pollable>();
        };
    }
};


/*
 * Represents the non-blocking asynchronous event loop that repeatedly calls poll() using
 * its internal list of pollables and processes them as events come in.
 */
class AsyncEventLoop {
    std::vector<std::shared_ptr<Pollable>> pollables;

    void process_pollable(std::shared_ptr<Pollable>, short revents);

public:
    void register_pollable(std::shared_ptr<Pollable> pollable);
    void loop();
};

#endif //ASYNC_EVENT_LOOP_H
