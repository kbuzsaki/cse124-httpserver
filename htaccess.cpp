#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <arpa/inet.h>
#include "htaccess.h"
#include "dns_client.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::runtime_error;
using std::shared_ptr;
using std::stoi;
using std::string;
using std::stringstream;
using std::vector;

#define BITS_IN_ADDRESS (32)

struct in_addr make_mask(int length) {
    uint32_t int_mask = 0xFFFFFFFF;

    if (length == 0) {
        int_mask = 0;
    } else {
        int_mask = int_mask << (BITS_IN_ADDRESS - length);
    }

    struct in_addr mask;
    mask.s_addr = htonl(int_mask);
    return mask;
}

CidrBlock::CidrBlock(struct in_addr prefix, int length) : prefix(prefix), length(length) {
    if ((prefix.s_addr & this->mask().s_addr) != prefix.s_addr) {
        stringstream error;
        error << "attempting to instantiate invalid cidr block: " << *this;
        throw runtime_error(error.str());
    }
}

bool CidrBlock::matches(struct in_addr address) {
    in_addr_t masked_address = address.s_addr & this->mask().s_addr;
    return this->prefix.s_addr == masked_address;
}

std::ostream& operator<<(std::ostream& os, const CidrBlock& block) {
    return os << "{" << block.prefix << "/" << block.length << "}";
}

struct in_addr CidrBlock::mask() {
    return make_mask(this->length);
}

struct in_addr parse_ip(string ip_str) {
    struct in_addr addr;

    int err = inet_pton(AF_INET, ip_str.c_str(), &addr);
    if (err < 0) {
        throw runtime_error(errno_message("inet_pton() failed for address '" + ip_str + "': "));
    } else if (err == 0) {
        throw runtime_error("inet_pton() failed for address '" + ip_str + "'");
    }

    return addr;
}

CidrBlock parse_cidr(string cidr_str) {
    vector<string> parts = split(cidr_str, "/");
    if (parts.size() != 2) {
        throw runtime_error("cidr string '" + cidr_str + "' does not have 2 parts");
    }

    struct in_addr prefix = parse_ip(parts[0]);
    int length = stoi(parts[1]);

    return CidrBlock(prefix, length);
}


HtAccess::HtAccess(vector<HtAccessRule> rules) : rules(rules) {}

bool HtAccess::allows(struct in_addr address) {
    for (size_t i = 0; i < rules.size(); i++) {
        HtAccessRule& rule = rules[i];

        if (rule.pattern.matches(address)) {
            return rule.allow;
        }
    }

    return true;
}


bool parse_allow_deny(string allow_str) {
    if (allow_str == "allow") {
        return true;
    } else if (allow_str == "deny") {
        return false;
    } else {
        throw runtime_error("invalid allow/deny in htaccess: '" + allow_str + "'");
    }
}

vector<CidrBlock> parse_cidr_or_domain(string host_str, shared_ptr<DnsClient> dns_client) {
    try {
        return vector<CidrBlock>{parse_cidr(host_str)};
    } catch (runtime_error&) {
        // if we failed to parse the block, then try to use dns instead
        vector<CidrBlock> blocks;
        vector<struct in_addr> ips = dns_client->lookup(host_str);
        for (size_t i = 0; i < ips.size(); i++) {
            blocks.push_back(CidrBlock{ips[i], BITS_IN_ADDRESS});
        }
        return blocks;
    }
}

HtAccess parse_htaccess_rules(string rules_str, shared_ptr<DnsClient> dns_client) {
    vector<HtAccessRule> rules;

    vector<string> rules_lines = split(rules_str, "\n");
    for (size_t i = 0; i < rules_lines.size(); i++) {
        if (rules_lines[i] == "") {
            continue;
        }

        vector<string> rule_components = split(rules_lines[i], " from ");
        if (rule_components.size() != 2) {
            throw runtime_error("invalid rule line in htaccess: '" + rules_lines[i] + "'");
        }

        bool allow = parse_allow_deny(rule_components[0]);
        vector<CidrBlock> blocks = parse_cidr_or_domain(rule_components[1], dns_client);

        for (size_t j = 0; j < blocks.size(); j++) {
            rules.push_back(HtAccessRule{blocks[j], allow});
        }
    }

    return HtAccess(rules);
}
