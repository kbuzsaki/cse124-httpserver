#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>

class File {
public:
    virtual ~File() {};

    virtual bool world_readable() = 0;
    virtual std::string contents() = 0;
};


class FileRepository {
public:
    virtual ~FileRepository() {};

    virtual std::shared_ptr<File> get_file(std::string path) = 0;
};


class PathFile : public File {
    std::string file_path;

public:
    PathFile(std::string file_path);

    virtual bool world_readable();
    virtual std::string contents();
};


class DirectoryFileRepository : public FileRepository {
    std::string directory_path;

public:
    DirectoryFileRepository(std::string directory_path);

    virtual std::shared_ptr<File> get_file(std::string path);
};

#endif //FILE_SYSTEM_H
