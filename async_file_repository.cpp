#include "async_file_repository.h"

using std::chrono::system_clock;
using std::make_shared;
using std::shared_ptr;
using std::string;

PathAsyncFile::PathAsyncFile(string file_path) : file_path(file_path) {}

shared_ptr<Pollable> PathAsyncFile::is_world_readable(Callback<bool>::F callback) {
    return callback(true);
}

shared_ptr<Pollable> PathAsyncFile::read_contents(Callback<string>::F callback) {
    return callback("TODO\n");
}

shared_ptr<Pollable> PathAsyncFile::read_last_modified(Callback<system_clock::time_point>::F callback) {
    return callback(system_clock::time_point());
}


DirectoryAsyncFileRepository::DirectoryAsyncFileRepository(string directory_path) : directory_path(directory_path) {}

shared_ptr<Pollable> DirectoryAsyncFileRepository::read_file(string filename, Callback<shared_ptr<AsyncFile>>::F callback) {
    return callback(make_shared<PathAsyncFile>(directory_path + "/" + filename));
}
