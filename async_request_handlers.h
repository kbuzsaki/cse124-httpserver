#ifndef ASYNC_REQUEST_HANDLERS_H_H
#define ASYNC_REQUEST_HANDLERS_H_H

#include "async_http_server.h"
#include "async_file_repository.h"
#include "async_request_filters.h"


class TestAsyncHttpRequestHandler : public AsyncHttpRequestHandler {
public:
    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F callback);
};


class FileServingAsyncHttpRequestHandler : public AsyncHttpRequestHandler {
    std::shared_ptr<AsyncFileRepository> repository;

public:
    FileServingAsyncHttpRequestHandler(std::shared_ptr<AsyncFileRepository> repository);

    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F callback);
};


class AsyncRequestFilterMiddleware : public AsyncHttpRequestHandler {
    std::shared_ptr<AsyncRequestFilter> filter;
    std::shared_ptr<AsyncHttpRequestHandler> handler;

public:
    AsyncRequestFilterMiddleware(std::shared_ptr<AsyncRequestFilter> filter, std::shared_ptr<AsyncHttpRequestHandler> handler);

    virtual std::shared_ptr<Pollable> handle_request(HttpRequest request, Callback<HttpResponse>::F callback);
};

#endif //ASYNC_REQUEST_HANDLERS_H_H
