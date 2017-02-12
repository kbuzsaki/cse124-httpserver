#ifndef ASYNC_REQUEST_FILTER_H
#define ASYNC_REQUEST_FILTER_H

#include "async_event_loop.h"
#include "async_file_repository.h"
#include "http.h"


/*
 * AsyncRequestFilter is an abstract class that represents an asynchronous predicate
 * for HttpRequests. It invokes its callback with an allow/deny boolean onces the result
 * is ready.
 */
class AsyncRequestFilter {
public:
    virtual std::shared_ptr<Pollable> allow_request(HttpRequest request, Callback<bool>::F callback) = 0;
};


/*
 * AsyncHtAccessRequestFilter filters requests like HtAccessRequestFilter from request_filters.h,
 * but in a non-blocking manner.
 */
class AsyncHtAccessRequestFilter : public AsyncRequestFilter {
    std::shared_ptr<AsyncFileRepository> repository;

public:
    AsyncHtAccessRequestFilter(std::shared_ptr<AsyncFileRepository>);

    virtual std::shared_ptr<Pollable> allow_request(HttpRequest request, Callback<bool>::F callback);
};

#endif //ASYNC_REQUEST_FILTER_H
