#include <iostream>
#include <stdexcept>
#include <sstream>
#include <arpa/inet.h>
#include "htaccess.h"
#include "util.h"

using std::cerr;
using std::endl;
using std::runtime_error;
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
    return os << "{" << format_ip(block.prefix) << "/" << block.length << "}";
}

struct in_addr CidrBlock::mask() {
    return make_mask(this->length);
}

std::string format_ip(struct in_addr address) {
    char buf[256];

    const char* ret = inet_ntop(AF_INET, &address, buf, sizeof(buf));
    if (ret == NULL) {
        throw runtime_error(errno_message("inet_ntop() failed: "));
    }

    return string(ret);
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


