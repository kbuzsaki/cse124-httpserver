#ifndef HTACCESS_H
#define HTACCESS_H

#include <iostream>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "dns_client.h"


/*
 * Represents a CIDR block prefix. Represented as an ip address prefix and length.
 * The `matches` method returns true if the given ip address is contained in this CIDR block.
 */
class CidrBlock {
    struct in_addr prefix;
    int length;

    struct in_addr mask();
public:
    CidrBlock(struct in_addr prefix, int length);

    bool matches(struct in_addr address);

    friend std::ostream& operator<<(std::ostream&, const CidrBlock&);
};
std::ostream& operator<<(std::ostream&, const CidrBlock&);

/*
 * Convenience functions for parsing ip addresses and cidr blocks from their string representations.
 */
struct in_addr parse_ip(std::string ip_str);
CidrBlock parse_cidr(std::string cidr_str);


/*
 * Represents a .htaccess rule either allowing or denying for a given cidr block.
 */
struct HtAccessRule {
    CidrBlock pattern;
    bool allow;
};


/*
 * Represents a list of HtACcessRules that either allow or deny a given ip address.
 * If the address doesn't match any of the rules, defaults to true.
 */
class HtAccess {
    std::vector<HtAccessRule> rules;

public:
    HtAccess(std::vector<HtAccessRule> rules);

    bool allows(struct in_addr address);
};


/*
 * Parses a string representation of .htaccess rules into an HtAccess object. If any
 * domain names are encountered, uses the provided DnsClient to look them up and treats them
 * as /32 CIDR blocks.
 */
HtAccess parse_htaccess_rules(std::string rule_str, std::shared_ptr<DnsClient> dns_client);

#endif //HTACCESS_H
