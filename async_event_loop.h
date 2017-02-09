#ifndef ASYNC_EVENT_LOOP_H
#define ASYNC_EVENT_LOOP_H

#include <functional>
#include <memory>
#include <vector>

class Pollable {
public:
    virtual ~Pollable() {};

    // get_fd returns the file descriptor for poll() to listen on
    virtual int get_fd() = 0;
    // get_events returns the events that poll() should check for
    virtual short get_events() = 0;

    virtual bool done() = 0;

    // TODO: add a timeout

    /**
     * notify is used to notify the Pollable that one of its events has occurred
     * @param revents  the events from poll() that occurred
     * @return another Pollable to listen to, or null if nothing more should be listened to
     */
    virtual std::shared_ptr<Pollable> notify(short revents) = 0;
};


// Convenience typedef to enable `Callback<string>::F` instead of `std::function<std::shared_ptr<Pollable> (string)>`
template<class ...Args>
struct Callback {
    typedef std::function<std::shared_ptr<Pollable> (Args...)> F;

    static F empty() {
        return [](Args...) -> std::shared_ptr<Pollable> {
            return std::shared_ptr<Pollable>();
        };
    }
};


class AsyncEventLoop {
    std::vector<std::shared_ptr<Pollable>> pollables;

    void prune();
public:
    void register_pollable(std::shared_ptr<Pollable> pollable);
    void loop();
};

#endif //ASYNC_EVENT_LOOP_H
