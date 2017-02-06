#ifndef REQUEST_FILTERS_H
#define REQUEST_FILTERS_H

#include <memory>
#include "dns_client.h"
#include "file_repository.h"
#include "htaccess.h"
#include "http.h"

class RequestFilter {
public:
    virtual bool allow_request(const HttpRequest& request) = 0;
};


class HtAccessRequestFilter : public RequestFilter {
    std::shared_ptr<FileRepository> repository;
    std::shared_ptr<DnsClient> dns_client;

public:
    HtAccessRequestFilter(std::shared_ptr<FileRepository>, std::shared_ptr<DnsClient>);

    virtual bool allow_request(const HttpRequest& request);
};

#endif //REQUEST_FILTERS_H
