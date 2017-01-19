#include "handlers.h"
#include "util.h"

using std::shared_ptr;
using std::string;

FileServingHttpHandler::FileServingHttpHandler(shared_ptr<FileRepository> repository) : repository(repository) {}

HttpResponse FileServingHttpHandler::handle_request(const HttpRequest& request) {
    // TODO: test this or move it into repository code
    string path = canonicalize_path(request.uri);
    if (path == "") {
        return not_found_response();
    }

    shared_ptr<File> file = repository->get_file(path);

    if (file == NULL) {
        return not_found_response();
    } else if (!file->world_readable()) {
        return forbidden_response();
    } else {
        return ok_response(file->contents());
    }
}
