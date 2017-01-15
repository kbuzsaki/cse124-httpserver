
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>

std::string pop_n_sstream(std::stringstream& buffer, size_t n);

size_t sstream_size(std::stringstream& buffer);
    
#endif //UTIL_H
