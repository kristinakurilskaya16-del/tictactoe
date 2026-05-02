#pragma once

#include "core/game.hpp"
#include <vector>

namespace ttt::my_player {

using game::Point;
using game::Sign;
using game::State;

struct Pattern {
    Sign sign;
    int length;
    int open_ends;
    bool has_gap = false;
    bool is_threat = false;

    Pattern() : sign(Sign::NONE), length(0), open_ends(0) {}

    Pattern(Sign s, int len, int ends, bool gap, bool threat) : sign(s), 
        length(len), open_ends(ends), has_gap(gap), is_threat(threat) {}

    bool is_relevant() const {
        return length >= 2 && open_ends > 0 && sign != Sign::NONE && 
            sign != Sign::WALL;
    }

    bool is_winning(int win_len) const {
        return length >= win_len && sign != Sign::NONE && 
            sign != Sign::WALL;
    }
};

struct Sequences {
    Pattern hor;
    Pattern ver;
    Pattern diag_rd;
    Pattern diag_ld;

    int score_for(Sign player, int win_len) const;
};

class SequencesAnalyzer {
private:
    

    static inline Sign safe_get(const State& state, int x, int y);

    static inline Pattern scan_one_direction(const State& state, int x, int y, 
                                    int dx, int dy, int win_len);
    static Pattern scan_direction(const State& state, int x, int y, 
                                int dx, int dy, Sign opponent, int win_len);

public:
   static Sequences analyze(const State& state, int x, int y, int win_len = 5);
   static std::vector<Point> find_all_threats(const State& state, Sign opponent, int win_len = 5);
};

}; // namespace ttt::my_player