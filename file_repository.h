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

#endif //FILE_SYSTEM_H
