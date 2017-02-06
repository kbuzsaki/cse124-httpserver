#ifndef SYNCHRONIZED_QUEUE_H
#define SYNCHRONIZED_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>


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
