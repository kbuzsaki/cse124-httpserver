#ifndef HANDLERS_H
#define HANDLERS_H

#include <memory>
#include "file_repository.h"
#include "server.h"

class FileServingHttpHandler : public HttpRequestHandler {
    std::shared_ptr<FileRepository> repository;

public:
    FileServingHttpHandler(std::shared_ptr<FileRepository>);

    virtual HttpResponse handle_request(const HttpRequest&);
};

#endif //HANDLERS_H
