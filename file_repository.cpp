#include <iostream>
#include <fstream>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file_repository.h"

using std::ifstream;
using std::istreambuf_iterator;
using std::make_shared;
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


DirectoryFileRepository::DirectoryFileRepository(std::string directory_path) : directory_path(directory_path) {}

std::shared_ptr<File> DirectoryFileRepository::get_file(std::string path) {
    string file_path = directory_path + "/" + path;
    if (access(file_path.c_str(), F_OK) < 0) {
        return NULL;
    }

    return make_shared<PathFile>(directory_path + "/" + path);
}
