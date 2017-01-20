
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i == 0) {
            os << "'" << v[i] << "'";
        } else {
            os << ", '" << v[i] << "'";
        }
    }
    return os << "]";
}

template<typename T>
std::string to_string(const T& val) {
    std::stringstream buf;
    buf << val;
    return buf.str();
}

std::vector<std::string> split(std::string s, std::string sep);

std::vector<std::string> split_n(std::string s, std::string sep, int n_splits);

std::string pop_n_sstream(std::stringstream& buffer, size_t n, size_t discard);

size_t sstream_size(std::stringstream& buffer);

std::string canonicalize_path(std::string path);

bool ends_with(std::string s, std::string suffix);

#endif //UTIL_H
