#include "request_filters.h"
#include <string>
#include <vector>
#include "util.h"

using std::shared_ptr;
using std::string;
using std::vector;


string htaccess_path_from_file_path(string path) {
    vector<string> parts = split(path, "/");
    if (parts.size() < 2) {
        return "";
    }

    parts.pop_back();
    parts.push_back(".htaccess");

    return join("/", parts);
}


HtAccessRequestFilter::HtAccessRequestFilter(std::shared_ptr<FileRepository> repository, std::shared_ptr<DnsClient> dns_client)
        : repository(repository), dns_client(dns_client) {}

bool HtAccessRequestFilter::allow_request(const HttpRequest& request) {
    string htacces_path = htaccess_path_from_file_path(canonicalize_path(request.uri));

    if (htacces_path != "") {
        shared_ptr<File> file = repository->get_file(htacces_path);

        if (file) {
            HtAccess htaccess = parse_htaccess_rules(file->contents(), dns_client);
            return htaccess.allows(request.remote_ip);
        }
    }

    return true;
}
