#ifndef MARKET_STRUCTURE_H
#define MARKET_STRUCTURE_H

/*
 * ============================================================
 *  CUSTOM MARKET STRUCTURE
 * ============================================================
 *
 *  4-BAR ROLLING WINDOW — every bar i, we evaluate:
 *    C1 = data[i-3]
 *    C2 = data[i-2]  ← label placed here (the peak or trough bar)
 *    C3 = data[i-1]
 *    C4 = data[i]    ← validator only (did price respect C2?)
 *
 *  PEAK PATTERN (C2 is the high):
 *    C1.high < C2.high   — C2 higher than C1
 *    C2.high > C3.high   — C3 drops from C2
 *    C4.high < C2.high   — C4 does not trade above C2
 *
 *  TROUGH PATTERN (C2 is the low):
 *    C1.low  > C2.low    — C2 lower than C1
 *    C2.low  < C3.low    — C3 bounces up from C2
 *    C4.low  > C2.low    — C4 does not trade below C2
 *
 *  C3 is unrestricted in both directions.
 *
 * ============================================================
 *  LABELING:
 *
 *  HIGHER HIGH (HH):
 *    Peak pattern fires AND C2.high > lastPeakC2_
 *    → emit HH, update lastPeakC2_ = C2.high
 *
 *  LOWER HIGH (LH):
 *    Peak pattern fires AND C2.high < lastPeakC2_
 *    AND hasHH_ and hasHL_ are true (prior HH + HL exist)
 *    → emit LH, update lastPeakC2_ = C2.high
 *
 *  HIGHER LOW (HL):
 *    Trough pattern fires AND C2.low > lastTroughC2_
 *    → emit HL, update lastTroughC2_ = C2.low, set hasHL_ = true
 *
 *  LOWER LOW (LL):
 *    Trough pattern fires AND C2.low < lastTroughC2_
 *    → emit LL, update lastTroughC2_ = C2.low, reset hasHL_ = false
 *
 * ============================================================
 *  CONSUMPTION RULE:
 *    After a pattern fires at C4=bar[i], store lastUsedC4_ = i.
 *    Next window: skip if C2 (i-2) or C3 (i-1) <= lastUsedC4_.
 *    C1 is allowed to equal lastUsedC4_ (it is reference only).
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

        // ── Bar 0: seed reference prices ────────────────────────────────────
        if (i == 0) {
            lastPeakC2_   = data[0].high;
            lastTroughC2_ = data[0].low;
            return;
        }

        // Need at least 4 bars
        if (i < 3) return;

        // ── Consumption check: C2 and C3 must be after lastUsedC4_ ──────────
        if ((i - 2) <= lastUsedC4_ ||
            (i - 1) <= lastUsedC4_)
            return;

        const Bar& c1    = data[i - 3];
        const Bar& c2    = data[i - 2];
        const Bar& c3    = data[i - 1];
        const Bar& c4    = data[i];
        int        c2Bar = i - 2;

        bool patternFired = false;

        // ====================================================================
        //  PEAK PATTERN
        // ====================================================================
        if (c1.high < c2.high &&   // C2 higher than C1
            c2.high > c3.high &&   // C3 drops from C2
            c4.high < c2.high)     // C4 doesn't break C2
        {
            if (c2.high > lastPeakC2_) {
                // New C2 above previous C2 → Higher High
                emit(StructureType::HH, c2.high, c2Bar);
                lastPeakC2_ = c2.high;
                hasHH_      = true;
                patternFired = true;

            } else if (c2.high < lastPeakC2_ && hasHH_ && hasHL_) {
                // New C2 below previous C2, prior HH+HL exist → Lower High
                emit(StructureType::LH, c2.high, c2Bar);
                lastPeakC2_ = c2.high;
                patternFired = true;
            }
        }

        // ====================================================================
        //  TROUGH PATTERN
        // ====================================================================
        if (c1.low  > c2.low  &&   // C2 lower than C1
            c2.low  < c3.low  &&   // C3 bounces from C2
            c4.low  > c2.low)      // C4 doesn't break C2
        {
            if (c2.low > lastTroughC2_) {
                // New C2 above previous trough C2 → Higher Low
                emit(StructureType::HL, c2.low, c2Bar);
                lastTroughC2_ = c2.low;
                hasHL_        = true;
                patternFired  = true;

            } else if (c2.low < lastTroughC2_) {
                // New C2 below previous trough C2 → Lower Low
                emit(StructureType::LL, c2.low, c2Bar);
                lastTroughC2_ = c2.low;
                hasHL_        = false;  // reset — need new HL before next LH
                patternFired  = true;
            }
        }

        if (patternFired) {
            lastUsedC4_ = i;
        }
    }

    double getLastPeakC2()   const { return lastPeakC2_; }
    double getLastTroughC2() const { return lastTroughC2_; }

private:
    double lastPeakC2_   = 0.0;  // C2.high of last confirmed peak pattern
    double lastTroughC2_ = 0.0;  // C2.low  of last confirmed trough pattern

    bool hasHH_ = false;  // have we seen at least one HH?
    bool hasHL_ = false;  // have we seen a HL after the last HH?

    int lastUsedC4_ = -1; // bar index of last consumed C4

    void emit(StructureType type, double price, int barIndex) {
        points.push_back({type, price, barIndex});
    }
};

#endif