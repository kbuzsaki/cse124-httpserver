#ifndef ASYNC_FILE_REPOSITORY_H
#define ASYNC_FILE_REPOSITORY_H

#include <chrono>
#include <memory>
#include <string>
#include "async_event_loop.h"

class AsyncFile {
public:
    virtual std::shared_ptr<Pollable> is_world_readable(Callback<bool>::F callback) = 0;
    virtual std::shared_ptr<Pollable> read_contents(Callback<std::string>::F callback) = 0;
    virtual std::shared_ptr<Pollable> read_last_modified(Callback<std::chrono::system_clock::time_point>::F callback) = 0;
};


class AsyncFileRepository {
public:
    virtual std::shared_ptr<Pollable> read_file(std::string filename, Callback<std::shared_ptr<AsyncFile>>::F callback) = 0;
};


class PathAsyncFile : public AsyncFile {
    std::string file_path;

public:
    PathAsyncFile(std::string file_path);

    virtual std::shared_ptr<Pollable> is_world_readable(Callback<bool>::F callback);
    virtual std::shared_ptr<Pollable> read_contents(Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> read_last_modified(Callback<std::chrono::system_clock::time_point>::F callback);
};


class DirectoryAsyncFileRepository : public AsyncFileRepository {
    std::string directory_path;

public:
    DirectoryAsyncFileRepository(std::string directory_path);

    virtual std::shared_ptr<Pollable> read_file(std::string filename, Callback<std::shared_ptr<AsyncFile>>::F callback);
};

#endif //ASYNC_FILE_REPOSITORY_H
