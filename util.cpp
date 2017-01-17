#include <algorithm>
#include <iostream>
#include "util.h"

using std::ios;
using std::min;
using std::string;
using std::stringstream;
using std::vector;

#define DEFAULT_READ_SIZE (100)
#define BUFFER_SIZE (5000)

vector<string> split(string s, string sep) {
    return split_n(s, sep, -1);
}

vector<string> split_n(string s, string sep, int n_splits) {
    vector<string> strs;

    size_t start = 0;
    size_t end = 0;
    while (true) {
        end = s.find(sep, start);

        if (end != string::npos && (n_splits < 0 || (strs.size() < (size_t) n_splits))) {
            strs.push_back(s.substr(start, end - start));
        } else {
            strs.push_back(s.substr(start, s.size()));
            return strs;
        }

        start = end + sep.size();
    }
}

string pop_n_sstream(stringstream& buffer, size_t n, size_t discard) {
    string buf_str = buffer.str();

    // the 'popped n' to return
    string return_string = buf_str.substr(0, min(n, buf_str.size()));

    // the remaining string, minus the popped string and the discarded string
    string remaining_string = buf_str.substr(min(n + discard, buf_str.size()), buf_str.size());

    // remove the string that we're reading from the buffer
    buffer.str(remaining_string);

    return return_string;
}

size_t sstream_size(std::stringstream& buffer) {
    size_t old_pos = buffer.tellg();
    buffer.seekg(0, ios::end);
    size_t size = buffer.tellg();
    buffer.seekg(old_pos, ios::beg);
    return size;
}


MockConnection::MockConnection(string payload) : read_payload(payload), write_payload(), read_size(DEFAULT_READ_SIZE), closed(false) {}

MockConnection::MockConnection(string payload, int read_size) : read_payload(payload), write_payload(), read_size(read_size), closed(false) {}

MockConnection::~MockConnection() {}

// TODO: make closed make these fail
std::string MockConnection::read() {
    char buf[BUFFER_SIZE];

    read_payload.read(buf, read_size);
    buf[read_payload.gcount()] = '\0';

    return string(buf);
}

void MockConnection::write(std::string s) {
    this->write_payload << s;
}

void MockConnection::close() {
    closed = true;
}

bool MockConnection::is_closed() {
    return closed;
}

std::string MockConnection::written() {
    return write_payload.str();
}
