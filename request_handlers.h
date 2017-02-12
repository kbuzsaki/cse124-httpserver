#ifndef HANDLERS_H
#define HANDLERS_H

#include <memory>
#include "file_repository.h"
#include "request_filters.h"
#include "server.h"


/*
 * FileServingHttpHandler serves incoming HttpRequests by looking for the file
 * matching the request uri in its FileRepository and responding based on the
 * characteristics of the file.
 * If the file is not present, it returns a 404 Not Found response.
 * IF the file is present but not world readable, it returns a 403 Forbidden response.
 * If the file is present and world readable, it returns a 200 OK response with the file's data.
 */
class FileServingHttpHandler : public HttpRequestHandler {
    std::shared_ptr<FileRepository> repository;

public:
    FileServingHttpHandler(std::shared_ptr<FileRepository>);

    virtual HttpResponse handle_request(const HttpRequest&);
};


/*
 * RequestFilterMiddleware is an HttpRequestHandler that takes a RequestFilter
 * predicate and an HttpRequestHandler to delegate to. When it receives an incoming
 * request, it asks the RequestFilter whether the request should be allowed. If the
 * RequestFilter denies the request, it returns a 403 Forbidden response. If the
 * RequestFilter allows the request, it delegates to the given HttpRequestHandler.
 */
class RequestFilterMiddleware : public HttpRequestHandler {
    std::shared_ptr<RequestFilter> filter;
    std::shared_ptr<HttpRequestHandler> handler;

public:
    RequestFilterMiddleware(std::shared_ptr<RequestFilter> filter, std::shared_ptr<HttpRequestHandler> handler);

    virtual HttpResponse handle_request(const HttpRequest&);
};

#endif //HANDLERS_H
