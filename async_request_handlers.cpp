#include "async_request_handlers.h"
#include <string>
#include "util.h"

using std::chrono::system_clock;
using std::shared_ptr;
using std::string;

string infer_content_type(string);


shared_ptr<Pollable> TestAsyncHttpRequestHandler::handle_request(HttpRequest, Callback<HttpResponse>::F callback) {
    return callback(forbidden_response());
}


FileServingAsyncHttpRequestHandler::FileServingAsyncHttpRequestHandler(shared_ptr<AsyncFileRepository> repository) : repository(repository) {}

shared_ptr<Pollable> FileServingAsyncHttpRequestHandler::handle_request(HttpRequest request, Callback<HttpResponse>::F callback) {
    string path = canonicalize_path(request.uri);
    if (path == "") {
        return callback(not_found_response());
    }

    if (path == "/") {
        path = "/index.html";
    }

    return repository->read_file(path, [=](shared_ptr<AsyncFile> file) -> shared_ptr<Pollable> {
        if (file == NULL) {
            return callback(not_found_response());
        }

        return file->is_world_readable([=](bool world_readable) -> shared_ptr<Pollable> {
            if (!world_readable) {
                return callback(forbidden_response());
            }

            return file->read_contents([=](string contents) -> shared_ptr<Pollable> {
                return file->read_last_modified([=](system_clock::time_point last_modified) -> shared_ptr<Pollable> {
                    return callback(ok_response(contents, infer_content_type(path), last_modified));
                });
            });
        });
    });
}


AsyncRequestFilterMiddleware::AsyncRequestFilterMiddleware(shared_ptr<AsyncRequestFilter> filter, shared_ptr<AsyncHttpRequestHandler> handler)
        : filter(filter), handler(handler) {}

shared_ptr<Pollable> AsyncRequestFilterMiddleware::handle_request(HttpRequest request, Callback<HttpResponse>::F callback) {
    return filter->allow_request(request, [=](bool allowed) -> shared_ptr<Pollable> {
        if (allowed) {
            return handler->handle_request(request, callback);
        } else {
            return callback(forbidden_response());
        }
    });
}
