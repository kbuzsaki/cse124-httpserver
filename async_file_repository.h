#ifndef ASYNC_FILE_REPOSITORY_H
#define ASYNC_FILE_REPOSITORY_H

#include <chrono>
#include <memory>
#include <string>
#include "async_event_loop.h"


/*
 * AsyncFile is an abstract class representing a unix file with asynchronous access operations.
 * It is analogous to File from file_repository.h, but asynchronous.
 */
class AsyncFile {
public:
    virtual std::shared_ptr<Pollable> is_world_readable(Callback<bool>::F callback) = 0;
    virtual std::shared_ptr<Pollable> read_contents(Callback<std::string>::F callback) = 0;
    virtual std::shared_ptr<Pollable> read_last_modified(Callback<std::chrono::system_clock::time_point>::F callback) = 0;
};


/*
 * AsyncFileRepository is an abstract class representing a repository of files with asynchronous access operations.
 * It is analogous to FileRepository from file_repository.h, but asynchronous.
 */
class AsyncFileRepository {
public:
    virtual std::shared_ptr<Pollable> read_file(std::string filename, Callback<std::shared_ptr<AsyncFile>>::F callback) = 0;
};


/*
 * PathAsyncFile represents a file at a given path like PathFIle from file_repository.h, but in an asynchronous manner.
 */
class PathAsyncFile : public AsyncFile {
    std::string file_path;

public:
    PathAsyncFile(std::string file_path);

    virtual std::shared_ptr<Pollable> is_world_readable(Callback<bool>::F callback);
    virtual std::shared_ptr<Pollable> read_contents(Callback<std::string>::F callback);
    virtual std::shared_ptr<Pollable> read_last_modified(Callback<std::chrono::system_clock::time_point>::F callback);
};


/*
 * DirectoryAsyncFileRepository represents a repository of files on the file system rooted at the
 * directory path. It is like DirectoryFileRepository from file_repository.h, but asynchrnous.
 */
class DirectoryAsyncFileRepository : public AsyncFileRepository {
    std::string directory_path;

public:
    DirectoryAsyncFileRepository(std::string directory_path);

    virtual std::shared_ptr<Pollable> read_file(std::string filename, Callback<std::shared_ptr<AsyncFile>>::F callback);
};

#endif //ASYNC_FILE_REPOSITORY_H
