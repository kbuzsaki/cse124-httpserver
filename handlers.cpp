#include "handlers.h"

using std::shared_ptr;

FileServingHttpHandler::FileServingHttpHandler(shared_ptr<FileRepository> repository) : repository(repository) {}

HttpResponse FileServingHttpHandler::handle_request(const HttpRequest& request) {
    shared_ptr<File> file = repository->get_file(request.uri);

    if (file == NULL) {
        return not_found_response();
    } else if (!file->world_readable()) {
        return forbidden_response();
    } else {
        return ok_response(file->contents());
    }
}
