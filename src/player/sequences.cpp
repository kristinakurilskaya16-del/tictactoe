#include "sequences.hpp"
#include <vector>
#include <array>

namespace ttt::my_player {  

int Sequences::score_for(Sign player, int win_len) const {
    int score = 0;
    const Pattern* patterns[] = {&hor, &ver, &diag_rd, &diag_ld};
    
    for (const auto* pat : patterns) {
        if (!pat->is_relevant() || pat->sign != player) continue;

        if (pat->is_threat) {
            return (pat->sign == player) ? +10000 : -10000;
        }
        
        static const int weights[5][3] = {
            // 0 ends, 1 end, 2 ends
            {    0,     1,     5},  // len=1
            {    0,    10,    50},  // len=2
            {    0,   100,   500},  // len=3
            {    0,  1000,  5000},  // len=4
            { 9999, 9999,  9999}    // len=5 (победа)
        };
        
        int w = weights[pat->length][pat->open_ends];
        if (pat->has_gap) 
            w /= 2;  
        score += w;
    }
    return score;
}


inline Sign SequencesAnalyzer::safe_get(const State& state, int x, int y) {
    if ( x < 0 || y < 0 || x >= state.get_opts().cols || y >= state.get_opts().rows)
        return Sign::WALL;
    return state.get_value(x, y);
}

// кеширование?
inline Pattern SequencesAnalyzer::scan_one_direction(const State& state, int x, int y, 
                                                int dx, int dy, int win_len) {
    
    int len = 0;
    bool open = false, gap = false;
    Sign sign = Sign::NONE;
    
    for (int i = 1; i <= win_len; ++i) {
        Sign s = safe_get(state, x + dx * i, y + dy * i);
        if (s == Sign::NONE) {
            open = true;

            if (len > 0 && safe_get(state, x + dx * (i + 1), y + dy* (i + 1)) == sign)
                gap = true;
            break;
        }
        if (s == Sign::WALL || (sign != Sign::NONE && s != sign)) break;
        sign = s;
        ++len;
    }
    return Pattern{sign, len, open ? 1 : 0, gap, false};
}

Pattern SequencesAnalyzer::scan_direction(const State& state, int x, int y, 
                                        int dx, int dy, Sign opponent, int win_len) {

    Pattern p = scan_one_direction(state, x, y,  dx,  dy, win_len);
    Pattern n = scan_one_direction(state, x, y, -dx, -dy, win_len);
    
    Pattern res;
    if (p.sign == n.sign && p.sign != Sign::NONE) {
        res = Pattern{p.sign, p.length + n.length, p.open_ends + n.open_ends, 
                      p.has_gap || n.has_gap, false};
    } else {
        res = (p.length >= n.length) ? p : n;
    }

    if (res.sign == opponent && res.length >= win_len - 1 && res.open_ends > 0) 
        res.is_threat = true;
    
    return res; 
}

Sequences SequencesAnalyzer::analyze(const State& state, int x, int y, int win_len) {
    Sign opponent = (state.get_current_player() == Sign::X) ? Sign::O : Sign::X;
    
    Sequences res;
    res.hor = scan_direction(state, x, y,  1,  0, opponent, win_len);
    res.ver = scan_direction(state, x, y,  0,  1, opponent, win_len);
    res.diag_rd = scan_direction(state, x, y,  -1, 1, opponent, win_len);
    res.diag_ld = scan_direction(state, x, y,  1, 1, opponent, win_len);
    return res;
}

std::vector<Point> SequencesAnalyzer::find_all_threats(const State& state, 
                                                    Sign opponent, int win_len) {
    
    std::vector<Point> threats;
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (state.get_value(x, y) != Sign::NONE) continue;
            
            bool near_activity = false;
            for (int dx = -2; dx <= 2 && !near_activity; ++dx) {
                for (int dy = -2; dy <= 2 && !near_activity; ++dy) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                        Sign val = state.get_value(nx, ny);

                        if (val == Sign::X || val == Sign::O) 
                            near_activity = true;
                    }
                }
            }
            if (!near_activity) continue;
            
            auto dirs = {Point{1,0}, Point{0,1}, Point{1,1}, Point{1,-1}};
            for (const auto& d : dirs) {
                Pattern pat = scan_direction(state, x, y, d.x, d.y, opponent, win_len);
                if (pat.is_threat){} 
                    threats.push_back({x, y});
                    break;
            }
        }
    }
    return threats;
}


} // namespace ttt::my_player