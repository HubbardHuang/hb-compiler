#include <cstdio>
#include <iostream>

#include "file_deleter.h"
#include "lexical_analyzer.h"

int
main(void) {
    std::unique_ptr<std::fstream, hcc::FileDeleter> file(
      new std::fstream("test/hello.c.test",
                       std::fstream::in | std::fstream::out),
      hcc::FileDeleter());
    hcc::LexicalAnalyzer::Instance().Work(file);
    return 0;
}