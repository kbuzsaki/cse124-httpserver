#include "util.h"

using std::ios;
using std::string;
using std::stringstream;
using std::vector;

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







