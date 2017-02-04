#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include <netinet/in.h>
#include <string>
#include <vector>

class DnsClient {
public:
    virtual ~DnsClient() {};

    virtual std::vector<struct in_addr> lookup(std::string domain) = 0;
};


class NetworkDnsClient : public DnsClient {
public:
    virtual std::vector<struct in_addr> lookup(std::string domain);
};

#endif //DNS_CLIENT_H
