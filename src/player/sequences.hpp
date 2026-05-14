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

    int get_pattern_weight() const;
    
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
    Pattern hor, ver, diag_rd, diag_ld;

    int score_for(Sign player, int win_len = 5) const;
};

class SequencesAnalyzer {
private:

    static inline Sign safe_get(const State& state, int x, int y) {
        if ( x < 0 || y < 0 || x >= state.get_opts().cols || y >= state.get_opts().rows)
            return Sign::WALL;
        return state.get_value(x, y);
    }

    static void scan_one_way(const State& state, int x, int y, 
                             int dx, int dy, Sign player, int win_len,
                             int& len, int& open_ends, bool& has_gap);

public:
    
    static Pattern scan_existing_line(const State& state, int x, int y, 
                                    int dx, int dy, int win_len);

    static Pattern evaluate_move_line(const State& state, int x, int y,
                        int dx, int dy, Sign player, int win_len);

};

struct ThreatInfo {
    std::vector<Point> winning_moves; // выигрыш в 1 ход
    std::vector<Point> blocking_moves; // срочный блок
    std::vector<Point> fork_moves; // двойные угрозы
    std::vector<Point> strong_moves; // open four / strong three
    int position_score = 0;
};

class ThreatEngine {
public:

    static ThreatInfo analyze(const State& state, Sign player, int win_len);

private:

    static bool is_near_activity(const State& state, int x, int y);

    static bool in_bounds(const State& state, int x, int y) {
        return x >= 0 && x < state.get_opts().cols && y >= 0 
            && y < state.get_opts().rows;
    }
};

}; // namespace ttt::my_player