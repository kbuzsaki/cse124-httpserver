#include <stdexcept>
#include "connection.h"
#include "http.h"
#include "util.h"

using std::chrono::system_clock;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

#define NUM_REQUEST_PARTS (3)
#define NUM_HEADER_PARTS (2)

#define CRLF ("\r\n")
#define CRLFCRLF ("\r\n\r\n")

vector<HttpHeader> parse_headers(const vector<string>&);


std::string HttpFrame::serialize() {
    return contents;
}

std::ostream& operator<<(std::ostream& os, const HttpFrame& frame) {
    return os << "{'" << frame.contents << "'}";
}

bool operator==(const HttpFrame& lhs, const HttpFrame& rhs) {
    return lhs.contents == rhs.contents;
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

// TODO: test this
bool has_header(const std::vector<HttpHeader>& headers, std::string key) {
    for (size_t i = 0; i < headers.size(); i++) {
        if (headers[i].key == key) {
            return true;
        }
    }

    return false;
}


HttpFrame HttpRequest::pack() {
    stringstream buf;

    buf << this->method << " " << this->uri << " " << this->version << CRLF;

    for (size_t i = 0; i < this->headers.size(); i++) {
        buf << this->headers[i].key << ": " << this->headers[i].value << CRLF;
    }

    buf << CRLF;

    buf << this->body;

    return HttpFrame{buf.str()};
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
    stringstream buf;

    buf << this->version << " " << this->status.code << " " << this->status.name << CRLF;

    for (size_t i = 0; i < this->headers.size(); i++) {
        buf << this->headers[i].key << ": " << this->headers[i].value << CRLF;
    }

    buf << CRLF;

    buf << this->body;

    return HttpFrame{buf.str()};
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


const HttpHeader SERVER_HEADER = HttpHeader{"Server", "TritonHTTP/0.1"};
const HttpHeader EMPTY_CONTENT_LENGTH = HttpHeader{"Content-Length", "0"};


HttpResponse ok_response(string body, string content_type, system_clock::time_point last_modified) {
    return HttpResponse{
            HTTP_VERSION_1_1,
            OK_STATUS,
            vector<HttpHeader>{
                    SERVER_HEADER,
                    HttpHeader{"Content-Length", to_string(body.size())},
                    HttpHeader{"Content-Type", content_type},
                    HttpHeader{"Last-Modified", to_http_date(last_modified)}
            },
            body
    };
}

HttpResponse error_response(HttpStatus status) {
    return HttpResponse{
            HTTP_VERSION_1_1,
            status,
            vector<HttpHeader>{SERVER_HEADER, EMPTY_CONTENT_LENGTH},
            ""
    };
}

HttpResponse bad_request_response() {
    return error_response(BAD_REQUEST_STATUS);
}

HttpResponse forbidden_response() {
    return error_response(FORBIDDEN_STATUS);
}

HttpResponse not_found_response() {
    return error_response(NOT_FOUND_STATUS);
}

HttpResponse internal_server_error_response() {
    return error_response(INTERNAL_SERVER_ERROR_STATUS);
}


HttpRequestParseError::HttpRequestParseError(string message) : runtime_error(message) {}


HttpConnection::HttpConnection(shared_ptr<Connection> conn) : conn(conn) {}

HttpConnection::HttpConnection(HttpConnection&& http_conn) : conn(std::move(http_conn.conn)) {}

HttpFrame HttpConnection::read_frame() {
    HttpFrame frame;

    frame.contents = conn.read_until(CRLFCRLF);

    return frame;
}

void HttpConnection::write_frame(HttpFrame frame) {
    this->conn.write(frame.serialize());
}

HttpRequest HttpConnection::read_request() {
    HttpFrame frame = this->read_frame();
    vector<string> frame_lines = split(frame.contents, CRLF);

    if (frame_lines.size() == 0) {
        throw HttpRequestParseError("malformed request missing initial line");
    }

    // first line is the request line / initial line
    string initial_line = frame_lines[0];
    // remaining lines are the headers
    frame_lines.erase(frame_lines.begin());

    vector<string>& header_lines = frame_lines;

    vector<string> parts = split_n(initial_line, " ", NUM_REQUEST_PARTS - 1);
    if (parts.size() != NUM_REQUEST_PARTS) {
        stringstream error;
        error << "malformed http request line '" << initial_line << "' had " << parts.size() << " parts, expected " << NUM_REQUEST_PARTS;
        throw HttpRequestParseError(error.str());
    }

    string method = parts[0];
    string uri = parts[1];
    string version = parts[2];
    vector<HttpHeader> headers = parse_headers(header_lines);

    return HttpRequest{method, uri, version, headers, "", {0}};
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

