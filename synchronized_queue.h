#ifndef SYNCHRONIZED_QUEUE_H
#define SYNCHRONIZED_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>


/*
 * SynchronizedQueue implements a templated thread-safe work queue.
 * The internal queue is protected from concurrent access by a mutex.
 * Calls to `push` acquire the mutex and add the element to the end of the queue.
 * Calls to `pop` retrieve and remove the first element of the queue if one exists,
 * or block the calling thread until an element is added.
 * It is safe for multiple threads to call push and pop concurrently.
 */
template <typename T>
class SynchronizedQueue {
    std::mutex lock;
    std::condition_variable cv;
    std::queue<T> queue;

public:
    SynchronizedQueue() {};

    void push(T elem) {
        std::lock_guard<std::mutex> guard(lock);
        queue.push(elem);
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> guard(lock);
        while (queue.size() == 0) {
            cv.wait(guard);
        }

        T elem = queue.front();
        queue.pop();

        return elem;
    }
};

#endif //SYNCHRONIZED_QUEUE_H
