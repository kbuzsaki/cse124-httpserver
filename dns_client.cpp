#include <netdb.h>
#include <stdexcept>
#include "dns_client.h"

using std::runtime_error;
using std::string;
using std::vector;

std::vector<struct in_addr> NetworkDnsClient::lookup(string domain) {
    vector<struct in_addr> addresses;

    struct addrinfo* result;
    struct addrinfo hint;
    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_INET;

    int ret = getaddrinfo(domain.c_str(), "domain", &hint, &result);
    if (ret != 0) {
        throw runtime_error(string("getaddrinfo() failed: ") + gai_strerror(ret));
    }

    for (struct addrinfo* n = result; n != NULL; n = n->ai_next) {
        struct sockaddr_in* sin = (struct sockaddr_in*) n->ai_addr;
        addresses.push_back(sin->sin_addr);
    }

    freeaddrinfo(result);

    return addresses;
}


vector<struct in_addr> NopDnsClient::lookup(string) {
    return vector<struct in_addr>();
}
