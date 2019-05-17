#ifndef FILE_DELETER
#define FILE_DELETER

#include <fstream>

namespace hcc {

class FileDeleter {
public:
    void operator()(std::fstream* fp) {
        fp->close();
        delete fp;
    }
    void operator()(std::ifstream* fp) {
        fp->close();
        delete fp;
    }
    void operator()(std::ofstream* fp) {
        fp->close();
        delete fp;
    }
};

} // namespace hcc

#endif