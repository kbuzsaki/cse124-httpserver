#ifndef HTACCESS_H
#define HTACCESS_H

#include <iostream>
#include <netinet/in.h>
#include <string>
#include <vector>

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

std::string format_ip(struct in_addr address);
struct in_addr parse_ip(std::string ip_str);
CidrBlock parse_cidr(std::string cidr_str);


struct HtAccessRule {
    CidrBlock pattern;
    bool allow;
};


class HtAccess {
    std::vector<HtAccessRule> rules;

public:
    HtAccess(std::vector<HtAccessRule> rules);

    bool allows(struct in_addr address);
};


HtAccess parse_htaccess_rules(std::string rule_str);

#endif //HTACCESS_H
