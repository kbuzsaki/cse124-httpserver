#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include <netinet/in.h>
#include <string>
#include <vector>

/*
 * DnsClient is an abstract class that can take a given domain string and return all of
 * the ip addresses associated with it.
 * It is implemented by NetworkDnsClient and NopDnsClient below, and by MockDnsClient in mocks.h.
 */
class DnsClient {
public:
    virtual ~DnsClient() {};

    virtual std::vector<struct in_addr> lookup(std::string domain) = 0;
};


/*
 * NetworkDnsClient implements DnsClient by calling `getaddrinfo` to make a
 * DNS request over the network.
 */
class NetworkDnsClient : public DnsClient {
public:
    virtual std::vector<struct in_addr> lookup(std::string domain);
};


/*
 * NopDnsClient implements DnsClient by returning an empty vector for every domain name.
 */
class NopDnsClient : public DnsClient {
public:
    virtual std::vector<struct in_addr> lookup(std::string domain);
};

#endif //DNS_CLIENT_H
