#include "request_handlers.h"
#include "util.h"

using std::shared_ptr;
using std::string;
using std::vector;

string infer_content_type(string);

FileServingHttpHandler::FileServingHttpHandler(shared_ptr<FileRepository> repository) : repository(repository) {}

HttpResponse FileServingHttpHandler::handle_request(const HttpRequest &request) {
    // TODO: test this or move it into repository code
    string path = canonicalize_path(request.uri);
    if (path == "") {
        return not_found_response();
    }

    if (path == "/") {
        path = "/index.html";
    }

    shared_ptr<File> file = repository->get_file(path);

    if (file == NULL) {
        return not_found_response();
    } else if (!file->world_readable()) {
        return forbidden_response();
    }

    HttpResponse response = ok_response(file->contents(), infer_content_type(path), file->last_modified());
    return response;
}

// TODO: add tests for this, maybe move it somewhere else
string infer_content_type(string filename) {
    if (ends_with(filename, ".html")) {
        return "text/html";
    } else if (ends_with(filename, ".jpg")) {
        return "image/jpeg";
    } else if (ends_with(filename, ".png")) {
        return "image/png";
    }
    return "text/plain";
}


RequestFilterMiddleware::RequestFilterMiddleware(std::shared_ptr<RequestFilter> filter, std::shared_ptr<HttpRequestHandler> handler)
        : filter(filter), handler(handler) {}

HttpResponse RequestFilterMiddleware::handle_request(const HttpRequest& request) {
    if (filter->allow_request(request)) {
        return handler->handle_request(request);
    }
    return forbidden_response();
}
