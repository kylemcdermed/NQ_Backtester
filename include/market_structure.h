#ifndef MARKET_STRUCTURE_H
#define MARKET_STRUCTURE_H

/*
 * ============================================================
 *  CUSTOM MARKET STRUCTURE
 * ============================================================
 *
 *  4-BAR ROLLING WINDOW — every bar i, we evaluate:
 *    C1 = data[i-3]
 *    C2 = data[i-2]  <- label placed here
 *    C3 = data[i-1]
 *    C4 = data[i]    <- validator only
 *
 *  PEAK PATTERN:
 *    C1.high < C2.high  &&  C2.high > C3.high  &&  C4.high < C2.high
 *
 *  TROUGH PATTERN:
 *    C1.low > C2.low  &&  C2.low < C3.low  &&  C4.low > C2.low
 *
 *  C3 unrestricted in both directions.
 *  Both peak AND trough can fire on the same C2 bar if both valid.
 *  No consumption rule — patterns fire freely every bar.
 *
 * ============================================================
 *  LABELING:
 *
 *  HH: peak fires AND C2.high > lastPeakC2_
 *      -> update lastPeakC2_, set hasHH_=true, inDowntrend_=false
 *
 *  LH: peak fires AND C2.high < lastPeakC2_
 *      AND hasHH_ AND (hasHL_ OR inDowntrend_)
 *      -> update lastPeakC2_, set inDowntrend_=true
 *
 *  HL: trough fires AND C2.low > lastTroughC2_
 *      -> update lastTroughC2_, set hasHL_=true, inDowntrend_=false
 *
 *  LL: trough fires AND C2.low < lastTroughC2_
 *      -> update lastTroughC2_, set hasHL_=false, inDowntrend_=true
 *
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

        // Seed reference prices from bar 0
        if (i == 0) {
            lastPeakC2_   = data[0].high;
            lastTroughC2_ = data[0].low;
            return;
        }

        if (i < 3) return;

        const Bar& c1    = data[i - 3];
        const Bar& c2    = data[i - 2];
        const Bar& c3    = data[i - 1];
        const Bar& c4    = data[i];
        int        c2Bar = i - 2;

        bool peakValid   = (c1.high < c2.high &&
                            c2.high > c3.high &&
                            c4.high < c2.high);

        bool troughValid = (c1.low  > c2.low  &&
                            c2.low  < c3.low  &&
                            c4.low  > c2.low);

        // ── PEAK pattern ─────────────────────────────────────────────────────
        if (peakValid) {
            if (c2.high > lastPeakC2_) {
                emit(StructureType::HH, c2.high, c2Bar);
                lastPeakC2_  = c2.high;
                hasHH_       = true;
                inDowntrend_ = false;

            } else if (c2.high < lastPeakC2_ && hasHH_ && (hasHL_ || inDowntrend_)) {
                emit(StructureType::LH, c2.high, c2Bar);
                lastPeakC2_  = c2.high;
                inDowntrend_ = true;
            }
        }

        // ── TROUGH pattern (fires independently of peak) ─────────────────────
        if (troughValid) {
            if (c2.low > lastTroughC2_) {
                emit(StructureType::HL, c2.low, c2Bar);
                lastTroughC2_ = c2.low;
                hasHL_        = true;
                inDowntrend_  = false;

            } else if (c2.low < lastTroughC2_) {
                emit(StructureType::LL, c2.low, c2Bar);
                lastTroughC2_ = c2.low;
                hasHL_        = false;
                inDowntrend_  = true;
            }
        }
    }

    double getLastPeakC2()   const { return lastPeakC2_; }
    double getLastTroughC2() const { return lastTroughC2_; }

private:
    double lastPeakC2_   = 0.0;
    double lastTroughC2_ = 0.0;

    bool hasHH_       = false;
    bool hasHL_       = false;
    bool inDowntrend_ = false;

    void emit(StructureType type, double price, int barIndex) {
        points.push_back({type, price, barIndex});
    }
};

#endif