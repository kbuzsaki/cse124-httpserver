#ifndef ASYNC_REQUEST_FILTER_H
#define ASYNC_REQUEST_FILTER_H

#include "async_event_loop.h"
#include "async_file_repository.h"
#include "http.h"

class AsyncRequestFilter {
public:
    virtual std::shared_ptr<Pollable> allow_request(HttpRequest request, Callback<bool>::F callback) = 0;
};


class AsyncHtAccessRequestFilter : public AsyncRequestFilter {
    std::shared_ptr<AsyncFileRepository> repository;

public:
    AsyncHtAccessRequestFilter(std::shared_ptr<AsyncFileRepository>);

    virtual std::shared_ptr<Pollable> allow_request(HttpRequest request, Callback<bool>::F callback);
};

#endif //ASYNC_REQUEST_FILTER_H
