#include "../include/parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

std::vector<Bar> CSVParser::loadData(const std::string& filename) {
    std::vector<Bar> data;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return data;
    }

    std::string line;
    if (!std::getline(file, line)) return data; // Skip header safely

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        Bar bar;

        try {
            std::getline(ss, bar.timestamp, ',');
            std::getline(ss, item, ','); bar.open = std::stod(item);
            std::getline(ss, item, ','); bar.high = std::stod(item);
            std::getline(ss, item, ','); bar.low = std::stod(item);
            std::getline(ss, item, ','); bar.close = std::stod(item);
            std::getline(ss, item, ','); bar.volume = std::stol(item);
            data.push_back(bar);
        } catch (...) {
            continue; // Skip malformed lines
        }
    }
    return data;
}