#ifndef MARKET_STRUCTURE_H
#define MARKET_STRUCTURE_H

/*
 * ============================================================
 *  CUSTOM MARKET STRUCTURE — context-aware labeling
 * ============================================================
 *
 *  TROUGH PATTERN — label decision:
 *    C2.low > lastHL  → HL  (trough above last confirmed HL)
 *    C2.low < lastHL  → LL  (broke below last HL = structure shift)
 *
 *  PEAK PATTERN — label decision:
 *    C2.high < lastLH → LH  (peak below last confirmed LH)
 *    C2.high > lastLH → HH  (broke above last LH = structure shift)
 *
 *  4-BAR WINDOW CONDITIONS (same for both):
 *    Trough: C1.low > C2.low  &&  C2.low < C3.low  &&  C4.low > C2.low
 *    Peak:   C1.high < C2.high  &&  C2.high > C3.high  &&  C4.high < C2.high
 *    C3 unrestricted in both directions.
 *
 *  Anchors initialized from bar 0.
 *  lastHL and lastLH update on every new HL/LH confirmed.
 * ============================================================
 */

#include <vector>
#include <string>
#include "parser.h"

enum class StructureType { NONE, HH, HL, LH, LL };

struct StructurePoint {
    StructureType type;
    double        price;
    int           barIndex;
};

class MarketStructure {
public:
    std::vector<StructurePoint> points;

    void update(const std::vector<Bar>& data, int i) {
        if (i >= (int)data.size()) return;

        if (i == 0) {
            lastHH_ = data[0].high;
            lastLH_ = data[0].high;
            lastHL_ = data[0].low;
            lastLL_ = data[0].low;
            return;
        }

        if (i < 3) return;

        const Bar& c1  = data[i - 3];
        const Bar& c2  = data[i - 2];
        const Bar& c3  = data[i - 1];
        const Bar& c4  = data[i];
        int        c2Bar = i - 2;

        // ── TROUGH pattern ───────────────────────────────────────────────────
        if (c1.low  > c2.low  &&
            c2.low  < c3.low  &&
            c4.low  > c2.low)
        {
            if (c2.low > lastHL_) {
                // Above last HL → new Higher Low
                emit(StructureType::HL, c2.low, c2Bar);
                lastHL_ = c2.low;
            } else if (c2.low < lastHL_) {
                // Below last HL → structure shift, Lower Low
                emit(StructureType::LL, c2.low, c2Bar);
                lastLL_ = c2.low;
                // Reset lastHL down so future troughs compare correctly
                lastHL_ = c2.low;
            }
        }

        // ── PEAK pattern ─────────────────────────────────────────────────────
        if (c1.high < c2.high &&
            c2.high > c3.high &&
            c4.high < c2.high)
        {
            if (c2.high < lastLH_) {
                // Below last LH → new Lower High
                emit(StructureType::LH, c2.high, c2Bar);
                lastLH_ = c2.high;
            } else if (c2.high > lastLH_) {
                // Above last LH → structure shift, Higher High
                emit(StructureType::HH, c2.high, c2Bar);
                lastHH_ = c2.high;
                // Reset lastLH up so future peaks compare correctly
                lastLH_ = c2.high;
            }
        }
    }

    double getLastHH() const { return lastHH_; }
    double getLastLL() const { return lastLL_; }
    double getLastHL() const { return lastHL_; }
    double getLastLH() const { return lastLH_; }

private:
    double lastHH_ = 0.0;
    double lastLH_ = 0.0;
    double lastHL_ = 0.0;
    double lastLL_ = 0.0;

    void emit(StructureType type, double price, int barIndex) {
        points.push_back({type, price, barIndex});
    }
};

#endif