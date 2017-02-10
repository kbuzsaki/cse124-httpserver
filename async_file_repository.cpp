#include "async_file_repository.h"
#include <iostream>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <sstream>
#include "util.h"

using std::chrono::system_clock;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::stringstream;

#define BUFSIZE (4096)


class FileReadPollable : public Pollable {
    int fd;
    Callback<string>::F callback;
    stringstream buffer;
    bool done;

    bool try_read() {
        char buf[BUFSIZE];

        while (true) {
            ssize_t ret = read(fd, buf, BUFSIZE);

            if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                return false;
            } else if (ret == -1) {
                std::cerr << errno_message("read() failed: ") << std::endl;
                return false;
            } else if (ret == 0) {
                return true;
            } else {
                buf[ret] = '\0';
                buffer << string(buf, (size_t)ret);
            }
        }
    }

public:
    FileReadPollable(string filename, Callback<string>::F callback) : callback(callback) {
        fd = open(filename.c_str(), O_NONBLOCK);
    }

    ~FileReadPollable() {
        close(fd);
    }

    virtual int get_fd() {
        return fd;
    }

    virtual short get_events() {
        return POLLIN;
    }

    virtual bool is_done() {
        return done;
    }

    virtual shared_ptr<Pollable> notify(short) {
        done = try_read();
        if (done) {
            return callback(buffer.str());
        } else {
            return shared_ptr<Pollable>();
        }
    }
};


PathAsyncFile::PathAsyncFile(string file_path) : file_path(file_path) {}

shared_ptr<Pollable> PathAsyncFile::is_world_readable(Callback<bool>::F callback) {
    // TODO: make this nonblocking?
    struct stat file_stat;
    ::stat(file_path.c_str(), &file_stat);
    return callback((file_stat.st_mode & S_IROTH));
}

shared_ptr<Pollable> PathAsyncFile::read_contents(Callback<string>::F callback) {
    return make_shared<FileReadPollable>(file_path, callback);
}

shared_ptr<Pollable> PathAsyncFile::read_last_modified(Callback<system_clock::time_point>::F callback) {
    // TODO: make this nonblocking?
    struct stat file_stat;
    ::stat(file_path.c_str(), &file_stat);
    return callback(to_time_point(file_stat.st_mtime));
}


DirectoryAsyncFileRepository::DirectoryAsyncFileRepository(string directory_path) : directory_path(directory_path) {}

shared_ptr<Pollable> DirectoryAsyncFileRepository::read_file(string filename, Callback<shared_ptr<AsyncFile>>::F callback) {
    // TODO: make this nonblocking?
    string file_path = directory_path + "/" + filename;
    if (access(file_path.c_str(), F_OK) < 0) {
        return callback(shared_ptr<PathAsyncFile>());
    }

    return callback(make_shared<PathAsyncFile>(file_path));
}
