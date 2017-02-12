#ifndef HTTP_H
#define HTTP_H

#include <chrono>
#include <netinet/in.h>
#include <stdexcept>
#include <vector>
#include "connection.h"

const std::string HTTP_VERSION_0_9 = "HTTP/0.9";
const std::string HTTP_VERSION_1_0 = "HTTP/1.0";
const std::string HTTP_VERSION_1_1 = "HTTP/1.1";


/*
 * HttpFrame represents a serialized HttpRequest or HttpResponse read from or
 * ready to be sent over a connection.
 * It can be parsed into an HttpRequest by the parse_request_frame() function below.
 * This is my "HTTPMessage" class, but I prefered this name.
 */
struct HttpFrame {
    std::string contents;

public:
    std::string serialize();
};
std::ostream& operator<<(std::ostream&, const HttpFrame&);
bool operator==(const HttpFrame&, const HttpFrame&);
bool operator!=(const HttpFrame&, const HttpFrame&);


/*
 * HttpHeader represents a single key-value pair header in an HttpRequest or HttpResponse.
 * A vector of headers can be accessed using the has_header and get_header functions below.
 */
struct HttpHeader {
    std::string key;
    std::string value;
};
std::ostream& operator<<(std::ostream&, const HttpHeader&);
bool operator==(const HttpHeader&, const HttpHeader&);
bool operator!=(const HttpHeader&, const HttpHeader&);

bool has_header(const std::vector<HttpHeader>& headers, std::string key);
HttpHeader get_header(const std::vector<HttpHeader>& headers, std::string key);


/*
 * HttpRequest represents a parsed http request. It can be parsed from an HttpFrame using
 * parse_request_frame below. It can be serialized into an HttpFrame with its `pack` method.
 * Validation of the request is done in parse_request_frame, so any HttpRequest objects
 * passed around in the server are expected to be valid.
 */
struct HttpRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::vector<HttpHeader> headers;
    std::string body;

    struct in_addr remote_ip;

public:
    HttpFrame pack();
};
std::ostream& operator<<(std::ostream&, const HttpRequest&);
bool operator==(const HttpRequest&, const HttpRequest&);
bool operator!=(const HttpRequest&, const HttpRequest&);


/*
 * HttpStatus represents a status code and message. Common status codes used
 * in the server are declared as constants below.
 */
struct HttpStatus {
    int code;
    std::string name;
};
std::ostream& operator<<(std::ostream&, const HttpStatus&);
bool operator==(const HttpStatus&, const HttpStatus&);
bool operator!=(const HttpStatus&, const HttpStatus&);


/*
 * HttpResponse represents an http response ready to be serialized and sent over
 * a connection. The `pack` method will serialize it into an HttpFrame.
 * HttpResponse objects should be constructed by HttpRequestHandlers and returned
 * so that it can be sent over the HttpConnection. Helper functions for constructing
 * common responses are declared below.
 */
struct HttpResponse {
    std::string version;
    HttpStatus status;
    std::vector<HttpHeader> headers;
    std::string body;

public:
    HttpFrame pack();
};
std::ostream& operator<<(std::ostream&, const HttpResponse&);
bool operator==(const HttpResponse&, const HttpResponse&);
bool operator!=(const HttpResponse&, const HttpResponse&);


/*
 * HttpStatus constants for common statuses
 */
const HttpStatus OK_STATUS = HttpStatus{200, "OK"};
const HttpStatus BAD_REQUEST_STATUS = HttpStatus{400, "Bad Request"};
const HttpStatus FORBIDDEN_STATUS = HttpStatus{403, "Forbidden"};
const HttpStatus NOT_FOUND_STATUS = HttpStatus{404, "Not Found"};
const HttpStatus INTERNAL_SERVER_ERROR_STATUS = HttpStatus{500, "Internal Server Error"};

/*
 * Helper functions for constructing common responses
 */
HttpResponse ok_response(std::string body, std::string content_type, std::chrono::system_clock::time_point last_modified);
HttpResponse bad_request_response();
HttpResponse forbidden_response();
HttpResponse not_found_response();
HttpResponse internal_server_error_response();

/*
 * Helper function for infering content type based on the name of a file
 */
std::string infer_content_type(std::string filename);


class HttpRequestParseError : public std::runtime_error {
public:
    HttpRequestParseError(std::string message);
};


/*
 * Parses an HttpFrame object into an HttpRequest object. If the HttpFrame contains
 * an invalid or malformed request, throws HttpRequestParseError.
 */
HttpRequest parse_request_frame(const HttpFrame& frame);


#endif //HTTP_H
