#ifndef HTTP_H
#define HTTP_H

#include <exception>
#include <vector>
#include "connection.h"

const std::string HTTP_VERSION_0_9 = "HTTP/0.9";
const std::string HTTP_VERSION_1_0 = "HTTP/1.0";
const std::string HTTP_VERSION_1_1 = "HTTP/1.1";

struct HttpFrame {
    std::string initial_line;
    std::vector<std::string> header_lines;
    std::string body;

public:
    std::string serialize();
};
std::ostream& operator<<(std::ostream&, const HttpFrame&);
bool operator==(const HttpFrame&, const HttpFrame&);
bool operator!=(const HttpFrame&, const HttpFrame&);

struct HttpHeader {
    std::string key;
    std::string value;
};
std::ostream& operator<<(std::ostream&, const HttpHeader&);
bool operator==(const HttpHeader&, const HttpHeader&);
bool operator!=(const HttpHeader&, const HttpHeader&);

struct HttpRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::vector<HttpHeader> headers;
    std::string body;

public:
    HttpFrame pack();
};
std::ostream& operator<<(std::ostream&, const HttpRequest&);
bool operator==(const HttpRequest&, const HttpRequest&);
bool operator!=(const HttpRequest&, const HttpRequest&);

struct HttpStatus {
    int code;
    std::string name;
};
std::ostream& operator<<(std::ostream&, const HttpStatus&);
bool operator==(const HttpStatus&, const HttpStatus&);
bool operator!=(const HttpStatus&, const HttpStatus&);

const HttpStatus OK_STATUS = HttpStatus{200, "OK"};
const HttpStatus BAD_REQUEST_STATUS = HttpStatus{400, "Bad Request"};
const HttpStatus FORBIDDEN_STATUS = HttpStatus{403, "Forbidden"};
const HttpStatus NOT_FOUND_STATUS = HttpStatus{404, "Not Found"};
const HttpStatus INTERNAL_SERVER_ERROR_STATUS = HttpStatus{500, "Internal Server Error"};

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

HttpResponse bad_request_response();
HttpResponse not_found_response();
HttpResponse internal_server_error_response();


class HttpRequestParseError : public std::runtime_error {
public:
    HttpRequestParseError(std::string message);
};


class HttpConnection {
    BufferedConnection conn;

    HttpFrame read_frame();
    void write_frame(HttpFrame frame);

public:
    HttpConnection(Connection* conn);
    HttpRequest read_request();
    void write_response(HttpResponse);
};

#endif //HTTP_H
