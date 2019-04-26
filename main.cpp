#include "lexical_analyzer.h"
#include <cstdio>
#include <iostream>

struct FileDeleter {
    void operator()(std::fstream* fp) {
        fp->close();
        delete[] fp;
    }
};

int
main(void) {
    std::shared_ptr<std::fstream> file(
      new std::fstream("test/hello.c.test", std::fstream::in | std::fstream::out));
    hcc::LexicalAnalyzer::Instance().Work(file);
    file->close();
    return 0;
}