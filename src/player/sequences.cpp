#include "sequences.hpp"
#include "weight.hpp"
#include <vector>
#include <array>

namespace ttt::my_player {  

int Pattern::get_pattern_weight() const {
    if (sign == Sign::NONE) return 0;
   
    if (length >= 5) return WEIGHT_WIN;

    if (length == 4) {
        if (open_ends == 2 && !has_gap) return WEIGHT_OPEN_FOUR;      
        if (open_ends == 1 && !has_gap) return WEIGHT_HALF_FOUR;      
        if (has_gap) return WEIGHT_GAP_FOUR;                           
    }
   
    if (length == 3) {
        if (open_ends == 2 && !has_gap) return WEIGHT_OPEN_THREE;    
        if (open_ends == 1 && !has_gap) return WEIGHT_HALF_THREE;      
        if (has_gap) return WEIGHT_GAP_THREE;                         
    }
    
    if (length == 2) {
        if (open_ends == 2 && !has_gap) return WEIGHT_OPEN_TWO;        
        if (open_ends >= 1 && !has_gap) return WEIGHT_HALF_TWO;      
        if (has_gap) return WEIGHT_GAP_TWO;                            
    }
    
    if (length == 1) return WEIGHT_POSITION;
    
    return 0;
}

int Sequences::score_for(Sign player, int win_len) const {
    int score = 0;
    const Pattern* patterns[] = {&hor, &ver, &diag_rd, &diag_ld};
    
    for (const auto* pat : patterns) {
        if (pat->sign != player) continue;
        
        int weight = pat->get_pattern_weight();
        
        if (player == pat->sign) 
            score += weight;
        else 
            score += weight * DEFENSE_MULTIPLIER / ATTACK_MULTIPLIER;
        
        if (pat->open_ends > 0) 
            score += WEIGHT_POSITION * pat->open_ends;

    }
    
    return score;
}

inline Pattern SequencesAnalyzer::scan_one_direction(const State& state, int x, int y, 
                                                int dx, int dy, int win_len) {
    
    int len = 0; 
    bool open = false, gap = false;
    Sign sign = Sign::NONE;

    for (int i = 1; i <= win_len; ++i) {
        Sign s = safe_get(state, x + dx * i, y + dy * i);
        if (s == Sign::NONE) {
            open = true;

            if (len > 0 && safe_get(state, x + dx * (i + 1), y + dy * (i + 1)) == sign)
                gap = true;
            break;
        }

        if (s == Sign::WALL || (sign != Sign::NONE && s != sign)) break;
        
        sign = s; 
        ++len;
    }
    return {sign, len, open ? 1 : 0, gap, false};
}

Pattern SequencesAnalyzer::scan_direction(const State& state, int x, int y, 
                                        int dx, int dy, Sign opponent, int win_len) {

    Pattern p = scan_one_direction(state, x, y,  dx,  dy, win_len);
    Pattern n = scan_one_direction(state, x, y, -dx, -dy, win_len);
    
    Pattern res;

    if (p.sign == n.sign && p.sign != Sign::NONE) {
        res = {p.sign, p.length + n.length, p.open_ends + n.open_ends, 
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
    
    return {
        scan_direction(state, x, y,  1,  0, opponent, win_len),
        scan_direction(state, x, y,  0,  1, opponent, win_len),
        scan_direction(state, x, y,  1,  1, opponent, win_len),
        scan_direction(state, x, y,  -1, 1, opponent, win_len)
    };
}

std::vector<Point> SequencesAnalyzer::find_all_threats(const State& state, 
                                                    Sign opponent, int win_len) {
    
    std::vector<Point> threats;
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (state.get_value(x, y) != Sign::NONE) continue;

            bool near = false;

            for (int dy = -2; dy <= 2 && !near; ++dy)
                for (int dx = -2; dx <= 2 && !near; ++dx)
                    if (dx || dy) {
                        int nx = x + dx;
                        int ny = y + dy;

                        if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                            Sign v = state.get_value(nx, ny);
                            if (v == Sign::X || v == Sign::O) near = true;
                        }
                    }

            if (!near) continue;
            const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
            for (auto& d : dirs) {
                Pattern pat = scan_direction(state, x, y, d[0], d[1], opponent, win_len);
                if (pat.is_threat) { 
                    threats.push_back({x, y}); 
                    break; 
                }
            }
        }
    }
    return threats;
}

std::vector<Point> SequencesAnalyzer::find_blocking_moves(const State& state, Sign opponent, int win_len) {
    std::vector<Point> blocking_moves;
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (state.get_value(x, y) != Sign::NONE) continue;
            
            bool is_blocking = false;
            const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
            
            for (auto& d : dirs) {
                int len = 1;
                for (int step = 1; step < win_len; ++step) {
                    Sign s = safe_get(state, x + d[0]*step, y + d[1]*step);
                    if (s == opponent) len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                for (int step = 1; step < win_len; ++step) {
                    Sign s = safe_get(state, x - d[0]*step, y - d[1]*step);
                    if (s == opponent) len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                if (len >= 4) { 
                    is_blocking = true;
                    break;
                }
            }
            
            if (is_blocking) {
                blocking_moves.push_back({x, y});
            }
        }
    }
    return blocking_moves;
}

} // namespace ttt::my_player