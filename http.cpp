#include <stdexcept>
#include "connection.h"
#include "http.h"
#include "util.h"

using std::ostream;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

#define NUM_REQUEST_PARTS (3)
#define NUM_HEADER_PARTS (2)

#define CRLF ("\r\n")

vector<HttpHeader> parse_headers(const vector<string>&);


std::string HttpFrame::serialize() {
    stringstream buf;

    buf << this->initial_line << CRLF;
    for (size_t i = 0; i < this->header_lines.size(); i++) {
        buf << this->header_lines[i] << CRLF;
    }
    buf << CRLF;
    buf << this->body;

    return buf.str();
}

std::ostream& operator<<(std::ostream& os, const HttpFrame& frame) {
    return os << "{'" << frame.initial_line << "', " << frame.header_lines << ", '" << frame.body << "'}";
}

bool operator==(const HttpFrame& lhs, const HttpFrame& rhs) {
    return lhs.initial_line == rhs.initial_line && lhs.header_lines == rhs.header_lines && lhs.body == rhs.body;
}

bool operator!=(const HttpFrame& lhs, const HttpFrame& rhs) {
    return !(lhs == rhs);
}


ostream& operator<<(ostream& os, const HttpHeader& header) {
    return os << "'" << header.key << ": " << header.value << "'";
}

bool operator==(const HttpHeader& lhs, const HttpHeader& rhs) {
    return lhs.key == rhs.key && lhs.value == rhs.value;
}

bool operator!=(const HttpHeader& lhs, const HttpHeader& rhs) {
    return !(lhs == rhs);
}


HttpFrame HttpRequest::pack() {
    HttpFrame frame;

    stringstream buf;
    buf << this->method << " " << this->uri << " " << this->version;
    frame.initial_line = buf.str();

    for (size_t i = 0; i < this->headers.size(); i++) {
        buf.str("");
        buf << this->headers[i].key << ": " << this->headers[i].value;
        frame.header_lines.push_back(buf.str());
    }

    frame.body = this->body;

    return frame;
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& request) {
    return os << "{'" << request.method << "', '" << request.uri << "', '" << request.version << "', "
              << request.headers << ", '" << request.body << "'}";
}

bool operator==(const HttpRequest& lhs, const HttpRequest& rhs) {
    return lhs.method == rhs.method && lhs.uri == rhs.uri && lhs.version == rhs.version
           && lhs.headers == rhs.headers && lhs.body == rhs.body;
}

bool operator!=(const HttpRequest& lhs, const HttpRequest& rhs) {
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& os, const HttpStatus& status) {
    return os << "{" << status.code << ", '" << status.name << "'}";
}

bool operator==(const HttpStatus& lhs, const HttpStatus& rhs) {
    return lhs.code == rhs.code;
}

bool operator!=(const HttpStatus& lhs, const HttpStatus& rhs) {
    return !(lhs == rhs);
}


HttpFrame HttpResponse::pack() {
    HttpFrame frame;

    stringstream buf;
    buf << this->version << " " << this->status.code << " " << this->status.name;
    frame.initial_line = buf.str();

    for (size_t i = 0; i < this->headers.size(); i++) {
        buf.str("");
        buf << this->headers[i].key << ": " << this->headers[i].value;
        frame.header_lines.push_back(buf.str());
    }

    frame.body = this->body;

    return frame;
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& response) {
    return os << "{'" << response.version << "', " << response.status << ", " << response.headers << ", '" << response.body << "'}";
}

bool operator==(const HttpResponse& lhs, const HttpResponse& rhs) {
    return lhs.version == rhs.version && lhs.status == rhs.status && lhs.headers == rhs.headers && lhs.body == rhs.body;
}

bool operator!=(const HttpResponse& lhs, const HttpResponse& rhs) {
    return !(lhs == rhs);
}


HttpResponse not_found_response() {
    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = NOT_FOUND_STATUS;
    response.headers.push_back(HttpHeader{"Server", "TritonHTTP/0.1"});
    response.headers.push_back(HttpHeader{"Content-Length", "0"});
    response.body = "";
    return response;
}

HttpResponse bad_request_response() {
    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = BAD_REQUEST_STATUS;
    response.headers.push_back(HttpHeader{"Server", "TritonHTTP/0.1"});
    response.headers.push_back(HttpHeader{"Content-Length", "0"});
    response.body = "";
    return response;
}

HttpResponse internal_server_error_response() {
    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = INTERNAL_SERVER_ERROR_STATUS;
    response.headers.push_back(HttpHeader{"Server", "TritonHTTP/0.1"});
    response.headers.push_back(HttpHeader{"Content-Length", "0"});
    response.body = "";
    return response;
}


HttpRequestParseError::HttpRequestParseError(string message) : runtime_error(message) {}


HttpConnection::HttpConnection(shared_ptr<Connection> conn) : conn(conn) {}

HttpFrame HttpConnection::read_frame() {
    HttpFrame frame;

    frame.initial_line = this->conn.read_until(CRLF);

    while (true) {
        string header_line = this->conn.read_until(CRLF);
        if (header_line != "") {
            frame.header_lines.push_back(header_line);
        } else {
            break;
        }
    }

    frame.body = "";

    return frame;
}

void HttpConnection::write_frame(HttpFrame frame) {
    this->conn.write(frame.serialize());
}

HttpRequest HttpConnection::read_request() {
    HttpFrame frame = this->read_frame();

    HttpRequest request;

    vector<string> parts = split_n(frame.initial_line, " ", NUM_REQUEST_PARTS - 1);
    if (parts.size() != NUM_REQUEST_PARTS) {
        stringstream error;
        error << "malformed http request line '" << frame.initial_line << "' had " << parts.size() << " parts, expected " << NUM_REQUEST_PARTS;
        throw HttpRequestParseError(error.str());
    }
    request.method = parts[0];
    request.uri = parts[1];
    request.version = parts[2];

    request.headers = parse_headers(frame.header_lines);
    request.body = frame.body;

    return request;
}

void HttpConnection::write_response(HttpResponse response) {
    this->write_frame(response.pack());
}


vector<HttpHeader> parse_headers(const vector<string>& lines) {
    vector<HttpHeader> headers;

    for (size_t i = 0; i < lines.size(); i++) {
        vector<string> parts = split_n(lines[i], ": ", NUM_HEADER_PARTS - 1);
        if (parts.size() != NUM_HEADER_PARTS) {
            stringstream error;
            error << "malformed http header '" << lines[i] << "' had " << parts.size() << " parts, expected " << NUM_HEADER_PARTS;
            throw HttpRequestParseError(error.str());
        }

        HttpHeader header = {parts[0], parts[1]};
        headers.push_back(header);
    }

    return headers;
}

