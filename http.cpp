#include <stdexcept>
#include "connection.h"
#include "http.h"
#include "util.h"

using std::string;
using std::stringstream;
using std::vector;

#define NUM_REQUEST_PARTS (3)
#define NUM_HEADER_PARTS (2)

#define CRLF ("\r\n")

HttpResponse not_found_response() {
    HttpResponse response;
    response.version = HTTP_VERSION_1_1;
    response.status = NOT_FOUND_STATUS;
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

vector<HttpHeader> parse_headers(const vector<string>&);

HttpConnection::HttpConnection(BufferedConnection&& conn) : conn(std::move(conn)) {}

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
    stringstream buf;

    buf << frame.initial_line << CRLF;
    for (size_t i = 0; i < frame.header_lines.size(); i++) {
        buf << frame.header_lines[i] << CRLF;
    }
    buf << CRLF;
    buf << frame.body;

    this->conn.write(buf.str());
}

HttpRequest HttpConnection::read_request() {
    HttpFrame frame = this->read_frame();

    HttpRequest request;

    vector<string> parts = split_n(frame.initial_line, " ", NUM_REQUEST_PARTS - 1);
    if (parts.size() != NUM_REQUEST_PARTS) {
        stringstream error;
        error << "malformed http request line '" << frame.initial_line << "' had " << parts.size() << " parts, expected " << NUM_REQUEST_PARTS;
        throw std::runtime_error(error.str());
    }
    request.method = parts[0];
    request.uri = parts[1];
    request.version = parts[2];

    request.headers = parse_headers(frame.header_lines);
    request.body = frame.body;

    return request;
}

void HttpConnection::write_response(HttpResponse response) {
    HttpFrame frame;

    stringstream buf;
    buf << response.version << " " << response.status.code << " " << response.status.name;
    frame.initial_line = buf.str();

    for (size_t i = 0; i < response.headers.size(); i++) {
        buf.str("");
        buf << response.headers[i].key << ": " << response.headers[i].value;
        frame.header_lines.push_back(buf.str());
    }

    frame.body = response.body;

    this->write_frame(frame);
}

vector<HttpHeader> parse_headers(const vector<string>& lines) {
    vector<HttpHeader> headers;

    for (size_t i = 0; i < lines.size(); i++) {
        // TODO: fix this happening
        if (lines[i] == "") {
            continue;
        }

        vector<string> parts = split_n(lines[i], ": ", NUM_HEADER_PARTS - 1);
        if (parts.size() != NUM_HEADER_PARTS) {
            stringstream error;
            error << "malformed http header '" << lines[i] << "' had " << parts.size() << " parts, expected " << NUM_HEADER_PARTS;
            throw std::runtime_error(error.str());
        }

        HttpHeader header = {parts[0], parts[1]};
        headers.push_back(header);
    }

    return headers;
}

