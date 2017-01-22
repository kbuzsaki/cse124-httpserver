#include <stdexcept>
#include "connection_handlers.h"

using std::shared_ptr;
using std::exception;


BlockingHttpConnectionHandler::BlockingHttpConnectionHandler(shared_ptr<HttpRequestHandler> handler) : handler(handler) {}

void BlockingHttpConnectionHandler::handle_connection(HttpConnection&& conn) {
    try {
        HttpRequest request = conn.read_request();
        if (!has_header(request.headers, "Host")) {
            conn.write_response(bad_request_response());
        } else {
            HttpResponse response = handler->handle_request(request);
            conn.write_response(response);
        }
    } catch (HttpRequestParseError&) {
        conn.write_response(bad_request_response());
    } catch (exception&) {
        conn.write_response(internal_server_error_response());
    }
}
