#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct Bar {
    std::string timestamp;
    double open, high, low, close;
    long volume;
};

class CSVParser {
public:
    static std::vector<Bar> loadData(const std::string& filename);
};

#endif