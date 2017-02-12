#ifndef REQUEST_FILTERS_H
#define REQUEST_FILTERS_H

#include <memory>
#include "dns_client.h"
#include "file_repository.h"
#include "htaccess.h"
#include "http.h"


/*
 * RequestFilter represents an arbitrary predicate for whether to allow or deny
 * incoming HttpRequests.
 * If `allow_request` returns true, the request passes the filter and should be
 * processed. If it returns false, the request fails the filter and should be denied.
 */
class RequestFilter {
public:
    virtual bool allow_request(const HttpRequest& request) = 0;
};


/*
 * HtAccessRequestFilter filters requests by checking for a .htaccess file in the directory
 * that corresponds to the file that would be served and allowing or denying the request
 * based on the rules in the .htaccess file and the DnsClient given to the filter.
 */
class HtAccessRequestFilter : public RequestFilter {
    std::shared_ptr<FileRepository> repository;
    std::shared_ptr<DnsClient> dns_client;

public:
    HtAccessRequestFilter(std::shared_ptr<FileRepository>, std::shared_ptr<DnsClient>);

    virtual bool allow_request(const HttpRequest& request);
};

#endif //REQUEST_FILTERS_H
