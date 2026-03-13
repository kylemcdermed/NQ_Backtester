#include "include/parser.h"
#include "include/market_structure.h"
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <iomanip>

// UTC-4 offset in seconds (Eastern Daylight Time)
static const long long UTC_OFFSET = -4LL * 3600LL;

std::string formatTime(long long ts) {
    // Shift to UTC-4 before formatting
    std::time_t t = (std::time_t)(ts + UTC_OFFSET);
    struct tm tm_{};
#ifdef _WIN32
    gmtime_s(&tm_, &t);   // use gmtime after manual offset — avoids local TZ interference
#else
    gmtime_r(&t, &tm_);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", &tm_);
    return buf;
}

std::string typeStr(StructureType t) {
    switch (t) {
        case StructureType::HH: return "HH";
        case StructureType::HL: return "HL";
        case StructureType::LH: return "LH";
        case StructureType::LL: return "LL";
        default:                return "??";
    }
}

int main() {
    const std::string filePath =
        "C:/Users/kylem/Desktop/NQ_Backtester/nq_data/CME_MINI_NQ1!, 1.csv";

    std::vector<Bar> data = CSVParser::loadData(filePath);
    if (data.empty()) { std::cerr << "No data loaded.\n"; return 1; }

    MarketStructure ms;
    for (int i = 0; i < (int)data.size(); ++i)
        ms.update(data, i);

    // Sort chronologically by bar index
    auto pts = ms.points;
    std::stable_sort(pts.begin(), pts.end(),
        [](const StructurePoint& a, const StructurePoint& b){
            return a.barIndex < b.barIndex;
        });

    // Build a quick lookup: barIndex → StructurePoint (for printing inline)
    // We'll print ALL bars with OHLC, and append label if one exists at that bar
    // Map bar index to label string (last one wins if multiple — rare)
    std::vector<std::string> labelAt(data.size(), "");
    for (const auto& p : pts) {
        if (p.barIndex >= 0 && p.barIndex < (int)data.size()) {
            // Append if multiple labels land on same bar
            if (!labelAt[p.barIndex].empty()) labelAt[p.barIndex] += "/";
            labelAt[p.barIndex] += typeStr(p.type);
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "time      open      high      low      close      label\n";
    std::cout << "---------------------------------------------------------------------\n";

    for (int i = 0; i < (int)data.size(); ++i) {
        const Bar& b = data[i];
        long long ts = std::stoll(b.timestamp);

        std::cout << "[" << formatTime(ts) << "]  "
                  << std::setw(9) << b.open  << "  "
                  << std::setw(9) << b.high  << "  "
                  << std::setw(9) << b.low   << "  "
                  << std::setw(9) << b.close;

        if (!labelAt[i].empty())
            std::cout << "  <-- *** " << labelAt[i] << " ***";

        std::cout << "\n";
    }

    std::cout << "---------------------------------------------------------------------\n";
    std::cout << "Total labels : " << pts.size()     << "\n";
    // To this:
    std::cout << "Final HH     : " << ms.getLastHH() << "\n";
    std::cout << "Final LL     : " << ms.getLastLL() << "\n";
    return 0;
}