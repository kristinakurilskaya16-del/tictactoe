#include "sequences.hpp"
#include "weight.hpp"
#include <vector>
#include <array>

namespace ttt::my_player {  

int Pattern::get_pattern_weight() const {
    if (sign == Sign::NONE) return 0;
    if (length >= 5) return WEIGHT_WIN;

    // Четвёрки 
    if (length == 4) {
        if (!has_gap) {
            if (open_ends == 2) return WEIGHT_OPEN_FOUR;      
            if (open_ends == 1) return WEIGHT_HALF_FOUR;      
            return WEIGHT_CLOSED_FOUR;                         
        } else { // has_gap == true
            if (open_ends >= 2) return WEIGHT_GAP_FOUR_OPEN2; 
            if (open_ends == 1) return WEIGHT_GAP_FOUR_OPEN1; 
            return WEIGHT_GAP_FOUR_OPEN0;                      
        }
    }

    // Тройки 
    if (length == 3) {
        if (!has_gap) {
            if (open_ends == 2) return WEIGHT_OPEN_THREE;   
            if (open_ends == 1) return WEIGHT_HALF_THREE;     
            return WEIGHT_CLOSED_THREE;                        
        } else {
            if (open_ends >= 2) return WEIGHT_GAP_THREE_OPEN2; 
            if (open_ends == 1) return WEIGHT_GAP_THREE_OPEN1; 
            return WEIGHT_GAP_THREE_OPEN0;                      
        }
    }

    // Двойки
    if (length == 2) {
        if (!has_gap) {
            if (open_ends == 2) return WEIGHT_OPEN_TWO;       
            if (open_ends == 1) return WEIGHT_HALF_TWO;      
            return WEIGHT_CLOSED_TWO;                       
        } else {
            if (open_ends >= 2) return WEIGHT_GAP_TWO_OPEN2;  
            if (open_ends == 1) return WEIGHT_GAP_TWO_OPEN1;  
            return WEIGHT_GAP_TWO_OPEN0;                      
        }
    }

    // Одиночный камень
    if (length == 1) return WEIGHT_SINGLE;                 

    return 0;
}

int Sequences::score_for(Sign player, int win_len) const {
    int score = 0;
    const Pattern* patterns[] = {&hor, &ver, &diag_rd, &diag_ld};
    
    for (const auto* pat : patterns) {
        if (pat->sign != player) continue;
        score += pat->get_pattern_weight();
    }
    
    return score;
}

void SequencesAnalyzer::scan_one_way(const State& state, int x, int y, 
                                     int dx, int dy, Sign player, int win_len,
                                     int& len, int& open_ends, bool& has_gap) {
    bool gap_used = false;
    
    for (int i = 1; i <= win_len; ++i) {

        int nx = x + dx * i;
        int ny = y + dy * i;

        Sign s = safe_get(state, nx, ny);
        
        if (s == player) {
            len++;
            continue;
        }
        
        if (s == Sign::NONE) {
            if (!gap_used) {
                Sign next = safe_get(state, x + dx * (i + 1), y + dy * (i + 1));

                if (next == player) {
                    has_gap = true;
                    gap_used = true;
                    continue;
                }
            }

            open_ends++;
            break;
        }
        
        break; 
    }
}

// уже существуют 
Pattern SequencesAnalyzer::scan_existing_line(const State& state,
                    int x, int y, int dx, int dy, int win_len) {

    Sign sign = safe_get(state, x, y);

    if (sign == Sign::NONE || sign == Sign::WALL)
        return {Sign::NONE, 0, 0, false, false};

    int len = 0;
    int open_ends = 0;
    bool has_gap = false;

    // forward scan
    scan_one_way(state, x, y, dx, dy, sign, win_len, len, open_ends, has_gap);

    return {sign, len, open_ends, has_gap, false};
}

// на будущее (если поставить сюда камень)
Pattern SequencesAnalyzer::evaluate_move_line(const State& state,
    int x, int y, int dx, int dy, Sign player, int win_len) {

    if (safe_get(state, x, y) != Sign::NONE)
        return {Sign::NONE, 0, 0, false, false};

    int len = 1; 
    int open_ends = 0;
    bool has_gap = false;

    scan_one_way(state, x, y, dx, dy, player, win_len, len, open_ends, has_gap);
    scan_one_way(state, x, y, -dx, -dy, player, win_len, len, open_ends, has_gap);

    return {player, len, open_ends, has_gap, false};
}

bool ThreatEngine::is_near_activity(const State& state, int x, int y) {
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (!in_bounds(state, nx, ny)) continue;
            if (state.get_value(nx, ny) != Sign::NONE) return true;
        }
    }

    return false;
}

ThreatInfo ThreatEngine::analyze(const State& state, Sign player, int win_len) {
    ThreatInfo info;

    Sign opponent = (player == Sign::X) ? Sign::O : Sign::X;

    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;

    static const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {-1,1}};

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {

            if (state.get_value(x, y) != Sign::NONE)
                continue;

            if (!is_near_activity(state, x, y))
                continue;

            int fork_score = 0;
            bool win_found = false;
            bool block_found = false;

            for (auto& d : dirs) {
                Pattern move_self = SequencesAnalyzer::evaluate_move_line(
                    state, x, y, d[0], d[1], player, win_len);

                Pattern move_opp = SequencesAnalyzer::evaluate_move_line(
                    state, x, y, d[0], d[1], opponent, win_len);

                if (move_self.length >= win_len) {   
                    info.winning_moves.push_back({x, y});
                    win_found = true;
                }

                if (move_opp.length >= win_len) {
                    info.blocking_moves.push_back({x, y});
                    block_found = true;
                }                

                if (move_self.length == 4) {
                    if (move_self.has_gap) {
                        if (move_self.open_ends == 2) fork_score += 6;
                        else if (move_self.open_ends == 1) fork_score += 5;
                        else fork_score += 3;
                    } else {
                        if (move_self.open_ends == 2) fork_score += 8;
                        else if (move_self.open_ends == 1) fork_score += 5;
                    }
                }
                
                if (move_self.length == 3) {
                    if (move_self.has_gap) {
                        if (move_self.open_ends >= 2) fork_score += 4; 
                        else if (move_self.open_ends == 1) fork_score += 3;  
                    } else {
                        if (move_self.open_ends == 2) fork_score += 4;  
                        else if (move_self.open_ends == 1) fork_score += 2;  
                    }
                }

                if (move_opp.length == 4) {
                    if (move_opp.has_gap) {
                        if (move_opp.open_ends == 2) fork_score -= 4;
                        else if (move_opp.open_ends == 1) fork_score -= 3;
                        else fork_score -= 1;
                    } else {
                        if (move_opp.open_ends == 2) fork_score -= 6;
                        else if (move_opp.open_ends == 1) fork_score -= 4;
                    }
                }
                
                if (move_opp.length == 3) {
                    if (move_opp.has_gap) {
                        if (move_opp.open_ends >= 2) fork_score -= 3; 
                        else if (move_opp.open_ends == 1) fork_score -= 2;  
                    } else {
                        if (move_opp.open_ends == 2) fork_score -= 3;  
                        else if (move_opp.open_ends == 1) fork_score -= 1;   
                    }
                }
            }

            if (fork_score >= 10 && !win_found && !block_found)
                info.fork_moves.push_back({x, y});

            info.position_score += fork_score;
        }
    }

    return info;
}

} // namespace ttt::my_player