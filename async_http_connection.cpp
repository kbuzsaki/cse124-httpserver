#include "async_http_connection.h"
#include <memory>

using std::exception;
using std::shared_ptr;
using std::string;

#define CRLFCRLF ("\r\n\r\n")


AsyncHttpConnection::AsyncHttpConnection(std::shared_ptr<AsyncSocketConnection> conn) : conn(conn) {}

std::shared_ptr<Pollable> AsyncHttpConnection::read_request(Callback<HttpRequest>::F callback) {
    return conn.read_until(CRLFCRLF, [=](string content) -> shared_ptr<Pollable> {
        HttpFrame frame = HttpFrame{content};

        try {
            HttpRequest request = parse_request_frame(frame);
            request.remote_ip = conn.get_remote_ip();
            return callback(request);
        } catch (HttpRequestParseError& r) {
            return write_response(bad_request_response(), Callback<>::empty());
        } catch (ConnectionClosed&) {
        } catch (exception& e) {
            return write_response(internal_server_error_response(), Callback<>::empty());
        }

        return shared_ptr<Pollable>();
    });
}

std::shared_ptr<Pollable> AsyncHttpConnection::write_response(HttpResponse response, Callback<>::F callback) {
    return conn.write(response.pack().serialize(), callback);
}


