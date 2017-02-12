#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <chrono>
#include <memory>
#include <string>


/*
 * File is an abstract class representing a unix file.
 * It provides accessors for the properties necessary to implement FileServingHttpHandler.
 * It is implemented below by PathFile and by MockFile in mocks.h
 */
class File {
public:
    virtual ~File() {};

    virtual bool world_readable() = 0;
    virtual std::string contents() = 0;
    virtual std::chrono::system_clock::time_point last_modified() = 0;
};


/*
 * FileRepository is an abstract class representing a repository of files. It provides
 * an accessor for looking up files by a given path string. If no such file exists, it returns
 * NULL.
 * It is implemented below by DirectoryFileRepository and by MockFileRepository in mocks.h
 * Other possible implementations include a client to a remote file store like S3, a large file store
 * like HDFS, or an in memory caching layer wrapping another file store.
 */
class FileRepository {
public:
    virtual ~FileRepository() {};

    virtual std::shared_ptr<File> get_file(std::string path) = 0;
};


/*
 * PathFile implements File by performing OS file system operations on the file at
 * the given file path.
 */
class PathFile : public File {
    std::string file_path;

public:
    PathFile(std::string file_path);

    virtual bool world_readable();
    virtual std::string contents();
    virtual std::chrono::system_clock::time_point last_modified();
};


/*
 * DirectoryFileRepository implements FileRepository by returning PathFiles with
 * file paths constructed by concatenating the directory path and the given path.
 */
class DirectoryFileRepository : public FileRepository {
    std::string directory_path;

public:
    DirectoryFileRepository(std::string directory_path);

    virtual std::shared_ptr<File> get_file(std::string path);
};

#endif //FILE_SYSTEM_H
