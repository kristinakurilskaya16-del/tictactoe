#include "my_player.hpp"
#include "sequences.hpp"
#include "weight.hpp"
#include <cstdlib>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) {
    m_sign = sign;
}

const char *MyPlayer::get_name() const {
    return m_name;
}

Point MyPlayer::make_move(const State& state) {
    Sign opponent = (m_sign == Sign::X) ? Sign::O : Sign::X;

    // Tactical scan 
    ThreatInfo my_info  = ThreatEngine::analyze(state, m_sign, 5);
    ThreatInfo opp_info = ThreatEngine::analyze(state, opponent, 5);

    // Immediate win
    for (const auto& w : my_info.winning_moves) {
        if (square_is_free(state, w.x, w.y))
            return w;
    }
        
    // Immediate block
    auto candidates = get_candidate_moves(state);
    for (const auto& c : candidates) {
        State tmp = state;
        tmp.process_move(opponent, c.x, c.y);
        if (would_win_fast(tmp, c.x, c.y, opponent)) return c;
    }

    // Start
    if (state.get_move_no() < 3) {
        Point start = find_best_start(state);
        if (start.x >= 0) return start;
    }

    // Forks
    for (const auto& f : my_info.fork_moves) {
        if (!square_is_free(state, f.x, f.y))
            continue;

        State tmp = state;
        tmp.process_move(m_sign, f.x, f.y);

        ThreatInfo after = ThreatEngine::analyze(tmp, opponent, 5);

        if (after.blocking_moves.size() >= 2)
            return f;
    }    

    // Move ordering
    auto moves = order_candidate_moves(state, m_sign, 10);

    if (moves.empty()) {
        for (int y = 0; y < state.get_opts().rows; ++y)
            for (int x = 0; x < state.get_opts().cols; ++x)
                if (square_is_free(state, x, y))
                    return {x, y};

        return {0, 0};
    }

    // Depth
    int free_cells = count_free_cells(state);

    int depth = (free_cells > 200) ? 2 : (free_cells > 120) ? 4 : 6;

    // Negamax
    Sign opponent_sign = (m_sign == Sign::X) ? Sign::O : Sign::X;

    Point best = moves[0];
    int best_value = -1e9;

    int alpha = -1e9;
    int beta  =  1e9;

    for (const auto& move : moves) {
        State next = state;
        next.process_move(m_sign, move.x, move.y);

        m_nodes_visited = 0;
        int value = -negamax(next, depth - 1, -beta, -alpha, opponent_sign, move.x, move.y);

        if (value > best_value) {
            best_value = value;
            best = move;
        }

        alpha = std::max(alpha, value);

        if (alpha >= beta)
            break;
    }

    if (!square_is_free(state, best.x, best.y)) {
        for (int y = 0; y < state.get_opts().rows; ++y)
            for (int x = 0; x < state.get_opts().cols; ++x)
                if (square_is_free(state, x, y))
                    return {x, y};

        return {0, 0};
    }

    return best;
}

int MyPlayer::evaluate_board_window(const State& state, Sign player) const {
     
    Sign opponent = (player == Sign::X) ? Sign::O : Sign::X;
    int score = 0;

    int cols = state.get_opts().cols;
    int rows = state.get_opts().rows;

    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {

            for (auto& d : dirs) {

                int my_cnt = 0;
                int opp_cnt = 0;
                bool blocked = false;

                for (int step = 0; step < 5; ++step) {

                    int nx = x + d[0]*step;
                    int ny = y + d[1]*step;

                    if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
                        blocked = true; 
                        break;
                    }

                    Sign s = state.get_value(nx, ny);

                    if (s == Sign::WALL) { blocked = true; break; }
                    if (s == player) my_cnt++;
                    else if (s == opponent) opp_cnt++;
                }

                if (blocked) continue;

                if (my_cnt > 0 && opp_cnt == 0)
                    score += line_value(my_cnt);
                else if (opp_cnt > 0 && my_cnt == 0)
                    score -= line_value(opp_cnt);
            }
        }
    }

    int cx = cols/2;
    int cy = rows/2;
    
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
            if (state.get_value(x, y) == player)
                score += (20 - (std::abs(x-cx) + std::abs(y-cy))) * 5;

    return score;
}

int MyPlayer::line_value(int count) const {
    if (count >= 5) return WEIGHT_WIN;          
    if (count == 4) return WEIGHT_HALF_FOUR;    
    if (count == 3) return WEIGHT_OPEN_THREE;   
    if (count == 2) return WEIGHT_OPEN_TWO;     
    if (count == 1) return WEIGHT_SINGLE; 
    return 0;
}

std::vector<Point> MyPlayer::order_candidate_moves(const State& state,
                                Sign player, int max_moves) const {

    Sign opponent = (player == Sign::X) ? Sign::O : Sign::X;

    std::vector<ScoredMove> scored;

    auto candidates = get_candidate_moves(state);

    scored.reserve(candidates.size());

    static const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {-1,1}};

    int cx = state.get_opts().cols / 2;
    int cy = state.get_opts().rows / 2;

    for (const auto& move : candidates) {
        int score = 0;

        for (auto& d : dirs) {
            Pattern self =
                SequencesAnalyzer::evaluate_move_line(
                    state,
                    move.x,
                    move.y,
                    d[0],
                    d[1],
                    player,
                    5
                );

            Pattern opp =
                SequencesAnalyzer::evaluate_move_line(
                    state,
                    move.x,
                    move.y,
                    d[0],
                    d[1],
                    opponent,
                    5
                );

            score += self.get_pattern_weight();

            score += opp.get_pattern_weight() * DEFENSE_FACTOR;

            if (self.length >= 5)
                score += 10'000'000;

            if (opp.length >= 5)
                score += 9'000'000;
        }

        int dx = move.x - cx;
        int dy = move.y - cy;
        int dist = (dx > 0 ? dx : -dx) + (dy > 0 ? dy : -dy);

        if (dist < 20) 
            score += (20 - dist) * 5;

        scored.push_back({move, score});
    }

    std::sort(scored.begin(),scored.end(),
        [](const auto& a, const auto& b) {
            return a.score > b.score;
        }
    );

    std::vector<Point> result;

    for (int i = 0;
         i < std::min(max_moves, (int)scored.size());
         ++i)
        result.push_back(scored[i].move);

    return result;
}

int MyPlayer::negamax(const State& state, int depth, int alpha,
    int beta, Sign current_player, int last_x, int last_y) const {
    
    if (++m_nodes_visited > MAX_NODES) 
        return evaluate_board_window(state, current_player);
    
    Sign opponent = (current_player == Sign::X) ? Sign::O : Sign::X;

    if (last_x >= 0 && last_y >= 0 && would_win_fast(state, last_x, last_y, opponent)) 
        return -10'000'000 + depth;   

    if (depth == 0)
        return evaluate_board_window(state, current_player);

    auto moves = order_candidate_moves(state, current_player, 8);

    if (moves.empty())
        return evaluate_board_window(state, current_player);

    int best = -2'000'000'000;

    for (const auto& move : moves) {
        if (!square_is_free(state, move.x, move.y))
            continue;

        State next = state;

        next.process_move(current_player, move.x, move.y);

        if (would_win_fast(next, move.x, move.y, current_player))
            return 10'000'000 - depth;

        int val = -negamax(next, depth - 1, -beta, -alpha, opponent);

        best = std::max(best, val);
        alpha = std::max(alpha, val);

        if (alpha >= beta)
            break;
    }

    return best;
}

bool MyPlayer::square_is_free(const State &state, int x, int y) const {
    return in_bounds(state, x, y) && state.get_value(x, y) == Sign::NONE;
}

bool MyPlayer::in_bounds(const State &state, int x, int y) const {
    return x >= 0 && x < state.get_opts().cols && y >= 0 && 
        y < state.get_opts().rows;
}

int MyPlayer::count_free_cells(const State& state) const {
    int count = 0;
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;

    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
            if (state.get_value(x, y) == Sign::NONE)
                count++;

    return count;
}

bool MyPlayer::has_neighbors(const State &state, int x, int y, int radius) const {
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;

    int min_x = std::max(0, x - radius);
    int max_x = std::min(cols - 1, x + radius);
    int min_y = std::max(0, y - radius);
    int max_y = std::min(rows - 1, y + radius);

    for (int ny = min_y; ny <= max_y; ++ny) {
        for (int nx = min_x; nx <= max_x; ++nx) {
            if (nx == x && ny == y) continue;
            Sign val = state.get_value(nx, ny);
            if (val == Sign::X || val == Sign::O) return true;
        }
    }

    return false;
}

Point MyPlayer::find_best_start(const State &state) const {
    int cx = state.get_opts().cols / 2;
    int cy = state.get_opts().rows / 2;

    if (square_is_free(state, cx, cy)) return {cx, cy};

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;

            int nx = cx + dx;
            int ny = cy + dy;

            if (square_is_free(state, nx, ny)) return {nx, ny};
        }
    }

    int max_radius = std::max({cx, state.get_opts().cols - 1 - cx,
                               cy, state.get_opts().rows - 1 - cy});

    for (int r = 2; r <= max_radius; ++r) {
        for (int dx = -r; dx <= r; ++dx) {
            for (int dy = -r; dy <= r; ++dy) {
                if (std::abs(dx) != r && std::abs(dy) != r) continue;

                int nx = cx + dx;
                int ny = cy + dy;

                if (square_is_free(state, nx, ny)) return {nx, ny};
            }
        }
    }

    return {cx, cy};
}

bool MyPlayer::would_win_fast(const State& state, int x, int y, Sign player) const {
  if (safe_get(state, x, y) != player)
    return false;

  const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};

  for (auto& d : dirs) {
    int count = 1;

    for (int step = 1; step < 5; ++step) {
      if (safe_get(state, x + d[0]*step, y + d[1]*step) == player)
        ++count;
      else break;
    }

    if (count >= 5) return true;

    for (int step = 1; step < 5; ++step) {
      if (safe_get(state, x - d[0]*step, y - d[1]*step) == player)
        ++count;
      else break;
    }

    if (count >= 5) return true;
  }

  return false;
}

Point MyPlayer::find_immediate_win_fast(const State& state) const {
    auto candidates = get_candidate_moves(state);
    for (const auto& c : candidates) {
        State tmp = state;
        tmp.process_move(m_sign, c.x, c.y);
        if (would_win_fast(tmp, c.x, c.y, m_sign)) return c;
    }

    return {-1, -1};
}

std::vector<Point> MyPlayer::get_candidate_moves(const State &state) const {

    std::vector<Point> moves;

    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {

            if (state.get_value(x, y) != Sign::NONE)
                continue;

            bool has_nbr = false;

            for (int dy = -2; dy <= 2 && !has_nbr; ++dy) {
                for (int dx = -2; dx <= 2 && !has_nbr; ++dx) {

                    if (dx == 0 && dy == 0) continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                        Sign v = state.get_value(nx, ny);
                        if (v == Sign::X || v == Sign::O) has_nbr = true;
                    }
                }
            }

            if (has_nbr) moves.push_back({x, y});
        }
    }

    if (moves.empty()) {
        int cx = cols / 2;
        int cy = rows / 2;

        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {

                int nx = cx + dx;
                int ny = cy + dy;

                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows)
                    moves.push_back({nx, ny});
            }
    }

    return moves;
}


} // namespace ttt::my_player