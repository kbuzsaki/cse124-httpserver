
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <vector>

std::vector<std::string> split(std::string s, std::string sep);

std::vector<std::string> split_n(std::string s, std::string sep, int n_splits);

std::string pop_n_sstream(std::stringstream& buffer, size_t n);

size_t sstream_size(std::stringstream& buffer);

    
#endif //UTIL_H
