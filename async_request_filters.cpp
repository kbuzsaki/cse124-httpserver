#include "async_request_filters.h"
#include "htaccess.h"
#include "dns_client.h"
#include "util.h"

using std::make_shared;
using std::shared_ptr;
using std::string;


string htaccess_path_from_file_path(string path);


AsyncHtAccessRequestFilter::AsyncHtAccessRequestFilter(shared_ptr<AsyncFileRepository> repository) : repository(repository) {}

shared_ptr<Pollable> AsyncHtAccessRequestFilter::allow_request(HttpRequest request, Callback<bool>::F callback) {
    string htacces_path = htaccess_path_from_file_path(canonicalize_path(request.uri));

    if (htacces_path != "") {
        return repository->read_file(htacces_path, [=](shared_ptr<AsyncFile> file) -> shared_ptr<Pollable> {
            if (file) {
                return file->read_contents([=](string contents) -> shared_ptr<Pollable> {
                    HtAccess htaccess = parse_htaccess_rules(contents, make_shared<NetworkDnsClient>());

                    return callback(htaccess.allows(request.remote_ip));
                });
            }

            return callback(true);
        });
    }

    return callback(true);
}
