#include <algorithm>
#include <ctime>
#include <iostream>
#include <string.h>
#include <stdexcept>
#include "util.h"

using std::chrono::system_clock;
using std::ios;
using std::max;
using std::min;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::vector;


vector<string> split(string s, string sep) {
    return split_n(s, sep, -1);
}

vector<string> split_n(string s, string sep, int n_splits) {
    vector<string> strs;

    size_t start = 0;
    size_t end = s.find(sep, start);
    // weird hack to prevent having an extra '' at the beginning with empty string separator
    if (sep == "") {
        end = 1;
    }

    while (true) {
        if (end < s.size() && (n_splits < 0 || (strs.size() < (size_t) n_splits))) {
            strs.push_back(s.substr(start, end - start));
        } else {
            strs.push_back(s.substr(start, s.size()));
            return strs;
        }

        start = end + sep.size();
        end = max(s.find(sep, start), (end + 1));
    }
}

string join(string joiner, vector<string> parts) {
    stringstream buf;

    for (size_t i = 0; i < parts.size(); i++) {
        if (i == 0) {
            buf << parts[i];
        } else {
            buf << joiner << parts[i];
        }
    }

    return buf.str();
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

string canonicalize_path(string path) {
    if (path == "") {
        return "";
    }

    vector<string> valid_parts;

    vector<string> parts = split(path, "/");
    for (size_t i = 0; i < parts.size(); i++) {
        string& part = parts[i];
        if (part == "") {
            continue;
        } else if (part == ".") {
            continue;
        } else if (part == "..") {
            if (valid_parts.size() == 0) {
                return "";
            } else {
                valid_parts.pop_back();
            }
        } else {
            valid_parts.push_back(part);
        }
    }

    return "/" + join("/", valid_parts);
}

bool ends_with(string s, string suffix) {
    if (suffix.size() > s.size()) {
        return false;
    }

    return s.substr(s.size() - suffix.size(), s.size()) == suffix;
}


system_clock::time_point make_time_point(int years, int months, int days, int hours, int minutes, int seconds) {
    struct tm t;
    bzero(&t, sizeof(t));

    // convert from absolute to epoch values
    t.tm_year = years - 1900;
    t.tm_mon = months - 1;
    t.tm_mday = days;
    t.tm_hour = hours;
    t.tm_min = minutes;
    t.tm_sec = seconds;

    time_t tt = timegm(&t);
    if (tt < 0) {
        stringstream error;
        error << "make_time_point(" << years << ", " << months << ", " << days << ", " << hours << ", " << minutes
              << ", " << seconds << "): " << strerror(errno);
        throw runtime_error(error.str());
    }

    return system_clock::from_time_t(tt);
}

system_clock::time_point to_time_point(time_t t) {
    system_clock::duration d = std::chrono::duration_cast<system_clock::duration>(std::chrono::seconds(t));
    return system_clock::time_point(d);
}

std::string to_http_date(const system_clock::time_point& tp) {
    char buf[1024];
    time_t t = system_clock::to_time_t(tp);
    struct tm tm = *gmtime(&t);
    // TODO: check this return value?
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

    // check for and replace utc suffix
    string s = string(buf);
    if (ends_with(s, "UTC")) {
        s.replace(s.size() - strlen("UTC"), strlen("UTC"), "GMT");
    }

    return s;
}

std::ostream &operator<<(std::ostream &os, const system_clock::time_point &tp) {
    return os << to_http_date(tp);
}


string errno_message(string prefix) {
    char buf[1024];
    bzero(buf, sizeof(buf));
    if (strerror_r(errno, buf, sizeof(buf)) != 0) {
        buf[0] = '\0';
    }

    stringstream error;
    error << prefix << buf;
    return error.str();
}

