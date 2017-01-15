#include "util.h"

using std::ios;
using std::string;
using std::stringstream;

string pop_n_sstream(stringstream& buffer, size_t n) {
    string buf_str = buffer.str();
    string return_string = buf_str.substr(0, n);

    // remove the string that we're reading from the buffer
    buffer.str(buf_str.substr(n, buf_str.size()));

    return return_string;
}

size_t sstream_size(std::stringstream& buffer) {
    size_t old_pos = buffer.tellg();
    buffer.seekg(0, ios::end);
    size_t size = buffer.tellg();
    buffer.seekg(old_pos, ios::beg);
    return size;
}







