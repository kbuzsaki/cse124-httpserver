#include <iostream>
#include <fstream>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file_repository.h"
#include "util.h"

using std::chrono::system_clock;
using std::ifstream;
using std::istreambuf_iterator;
using std::make_shared;
using std::shared_ptr;
using std::string;


PathFile::PathFile(std::string file_path) : file_path(file_path) {}

bool PathFile::world_readable() {
    struct stat file_stat;
    ::stat(file_path.c_str(), &file_stat);
    return (bool) (file_stat.st_mode & S_IROTH);
}

std::string PathFile::contents() {
    ifstream file_stream(file_path);
    return string(istreambuf_iterator<char>(file_stream), istreambuf_iterator<char>());
}

system_clock::time_point PathFile::last_modified() {
    struct stat file_stat;
    ::stat(file_path.c_str(), &file_stat);
    return to_time_point(file_stat.st_mtime);
}


DirectoryFileRepository::DirectoryFileRepository(std::string directory_path) : directory_path(directory_path) {}

std::shared_ptr<File> DirectoryFileRepository::get_file(std::string path) {
    string file_path = directory_path + "/" + path;
    if (access(file_path.c_str(), F_OK) < 0) {
        return shared_ptr<File>();
    }

    return make_shared<PathFile>(directory_path + "/" + path);
}
