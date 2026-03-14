#ifndef MARKET_STRUCTURE_H
#define MARKET_STRUCTURE_H

/*
 * ============================================================
 *  CUSTOM MARKET STRUCTURE — context-aware labeling
 * ============================================================
 *
 *  ROLLING 4-BAR WINDOW:
 *    C1 = data[i-3]
 *    C2 = data[i-2]   ← label placed here
 *    C3 = data[i-1]
 *    C4 = data[i]     ← validator bar, CONSUMED after pattern fires
 *
 *  CONSUMPTION RULE:
 *    When a pattern fires (trough or peak), C4's bar index is recorded
 *    as lastUsedC4_. Any subsequent window where C1, C2, or C3 would
 *    overlap with that bar is skipped entirely. This prevents C4 of one
 *    pattern from becoming part of the next pattern.
 *
 *    Concretely: after a pattern fires at bar i (C4=i, C2=i-2),
 *    the next valid window cannot start until C1 = i+1 at earliest,
 *    meaning the next pattern can fire at i+3 earliest (C4=i+3).
 *
 *  TROUGH PATTERN conditions:
 *    C1.low > C2.low  &&  C2.low < C3.low  &&  C4.low > C2.low
 *
 *    Label:
 *      C2.low > lastHL → HL, update lastHL
 *      C2.low < lastHL → LL (structure shift), update lastHL + lastLL
 *
 *  PEAK PATTERN conditions:
 *    C1.high < C2.high  &&  C2.high > C3.high  &&  C4.high < C2.high
 *
 *    Label:
 *      C2.high < lastLH → LH, update lastLH
 *      C2.high > lastLH → HH (structure shift), update lastLH + lastHH
 *
 *  C3 is unrestricted in both directions for both patterns.
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

        // ── Consumption check ────────────────────────────────────────────────
        // Skip this window if any of C1, C2, C3 overlaps with the last C4
        // lastUsedC4_ is the bar index of the C4 that last confirmed a pattern
        // C1=i-3, C2=i-2, C3=i-1 must all be > lastUsedC4_
        if ((i - 3) <= lastUsedC4_ ||
            (i - 2) <= lastUsedC4_ ||
            (i - 1) <= lastUsedC4_)
            return;

        bool troughFired = false;
        bool peakFired   = false;

        // ── TROUGH pattern ───────────────────────────────────────────────────
        if (c1.low  > c2.low  &&
            c2.low  < c3.low  &&
            c4.low  > c2.low)
        {
            if (c2.low > lastHL_) {
                emit(StructureType::HL, c2.low, c2Bar);
                lastHL_     = c2.low;
                troughFired = true;
            } else if (c2.low < lastHL_) {
                emit(StructureType::LL, c2.low, c2Bar);
                lastLL_     = c2.low;
                lastHL_     = c2.low;   // reset HL anchor to new LL
                troughFired = true;
            }
        }

        // ── PEAK pattern ─────────────────────────────────────────────────────
        if (c1.high < c2.high &&
            c2.high > c3.high &&
            c4.high < c2.high)
        {
            if (c2.high < lastLH_) {
                emit(StructureType::LH, c2.high, c2Bar);
                lastLH_   = c2.high;
                peakFired = true;
            } else if (c2.high > lastLH_) {
                emit(StructureType::HH, c2.high, c2Bar);
                lastHH_   = c2.high;
                lastLH_   = c2.high;    // reset LH anchor to new HH
                peakFired = true;
            }
        }

        // ── Record C4 consumption if any pattern fired ───────────────────────
        if (troughFired || peakFired) {
            lastUsedC4_ = i;  // C4 = current bar i is now consumed
        }
    }

    double getLastHH() const { return lastHH_; }
    double getLastLL() const { return lastLL_; }
    double getLastHL() const { return lastHL_; }
    double getLastLH() const { return lastLH_; }

private:
    double lastHH_     = 0.0;
    double lastLH_     = 0.0;
    double lastHL_     = 0.0;
    double lastLL_     = 0.0;
    int    lastUsedC4_ = -1;   // bar index of last consumed C4

    void emit(StructureType type, double price, int barIndex) {
        points.push_back({type, price, barIndex});
    }
};

#endif