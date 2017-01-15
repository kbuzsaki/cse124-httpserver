#include <stdexcept>
#include "connection.h"
#include "http.h"
#include "util.h"

using std::string;
using std::stringstream;
using std::vector;

#define NUM_REQUEST_PARTS (3)
#define NUM_HEADER_PARTS (2)

#define SEPARATOR ("\r\n")

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

HttpRequest HttpConnection::read_request() {
    HttpRequest request;
    
    string request_line = conn.read_until("\r\n");
    vector<string> parts = split_n(request_line, " ", NUM_REQUEST_PARTS - 1);
    if (parts.size() != NUM_REQUEST_PARTS) {
        stringstream error;
        error << "malformed http request line '" << request_line << "' had " << parts.size() << " parts, expected " << NUM_REQUEST_PARTS;
        throw std::runtime_error(error.str());
    }
    request.method = parts[0];
    request.uri = parts[1];
    request.version = parts[2];
    
    string request_body = conn.read_until("\r\n\r\n");
    request_body.erase(request_body.size() - 4, 4);
    request.headers = parse_headers(split(request_body, "\r\n"));
    
    return request;
}

void HttpConnection::write_response(HttpResponse response) {
    stringstream resp;
    
    resp << response.version << " " << response.status.code << " " << response.status.name << SEPARATOR;
    for (size_t i = 0; i < response.headers.size(); i++) {
        resp << response.headers[i].key << ": " << response.headers[i].value << SEPARATOR;
    }
    resp << SEPARATOR;
    resp << response.body;
    
    conn.write(resp.str());
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

