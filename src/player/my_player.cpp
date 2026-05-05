#include "my_player.hpp"
#include "weight.hpp"
#include <cstdlib>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State& state) {
  Sign opponent = (m_sign == Sign::X) ? Sign::O : Sign::X;
  
  // 1. Немедленная победа 
  if (Point win = find_immediate_win_fast(state); win.x >= 0) 
    return win;
  
  // 2. Первые ходы - в центр (ранняя агрессия)
  if (state.get_move_no() < 3) {
    Point start = find_best_start(state);
    if (start.x >= 0) return start;
  }
  
  // 3. Атака
  Point best = find_best_by_heuristic(state, opponent);
  if (best.x >= 0 && square_is_free(state, best.x, best.y)) 
    return best;
  
  // 4. Защита 
  if (Point block = find_immediate_block_fast(state, opponent); block.x >= 0) 
    return block;
  
  // 5. Блокировка разрывных угроз 
  if (Point gap_block = find_gap_block(state, opponent); gap_block.x >= 0)
    return gap_block;
  
  // 6. Стратегическая блокировка 
  if (Point strat = find_strategic_block(state, opponent); strat.x >= 0) 
    return strat;
  
  // 7. Почти конец
  int free_cells = count_free_cells(state);
  if (free_cells < 50) {
    return find_endgame_with_sim(state, opponent);
  }
  
  // 8. Fallback - любой ход
  for (int y = 0; y < state.get_opts().rows; ++y)
    for (int x = 0; x < state.get_opts().cols; ++x)
      if (square_is_free(state, x, y)) return {x, y};
  
  return {0, 0};
}

// ==================================================================

bool MyPlayer::square_is_free(const State &state, int x, int y) const {
  return in_bounds(state, x, y) && state.get_value(x, y) == Sign::NONE;
}

bool MyPlayer::in_bounds(const State &state, int x, int y) const {
  return x >= 0 && x < state.get_opts().cols && y >= 0 && y < state.get_opts().rows;
}

int MyPlayer::count_free_cells(const State& state) const {
  int count = 0;
  int rows = state.get_opts().rows;
  int cols = state.get_opts().cols;

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      if (state.get_value(x, y) == Sign::NONE) 
        count++;
    }
  }
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

      if (square_is_free(state, nx, ny)) 
        return {nx, ny};
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

        if (square_is_free(state, nx, ny)) 
          return {nx, ny};
      }
    }
  }

  return {cx, cy};
}

bool MyPlayer::would_win_fast(const State& state, int x, int y, 
                              Sign player) const {
  const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
  
  for (auto& d : dirs) {
    int count = 1;
    for (int step = 1; step < 5; ++step) {
      Sign s = state.get_value(x + d[0]*step, y + d[1]*step);
      if (s == player) count++;
      else if (s == Sign::WALL) break;
      else if (s == Sign::NONE) break;
      else break;
    }
    for (int step = 1; step < 5; ++step) {
      Sign s = state.get_value(x - d[0]*step, y - d[1]*step);
      if (s == player) count++;
      else if (s == Sign::WALL) break;
      else if (s == Sign::NONE) break;
      else break;
    }
    if (count >= 5) return true;
  }
  return false;
}

Point MyPlayer::find_immediate_win_fast(const State& state) const {
  auto candidates = get_candidate_moves(state);
  for (const auto& c : candidates) {
    if (would_win_fast(state, c.x, c.y, m_sign)) return c;
  }
  return {-1, -1};
}

Point MyPlayer::find_immediate_block_fast(const State& state, Sign opponent) const {
  auto candidates = get_candidate_moves(state);

  for (const auto& c : candidates) {
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (auto& d : dirs) {
      Pattern pat = SequencesAnalyzer::scan_direction(state, c.x, c.y, d[0], d[1], opponent, 5);
      
      if (pat.sign == opponent && pat.length >= 4) {
        return c;
      }
      
      if (pat.sign == opponent && pat.length >= 3 && pat.open_ends >= 1) {
        return c;
      }
      
      if (pat.sign == opponent && pat.length >= 3 && pat.has_gap) {
        return c;
      }
    }
  }

  return {-1, -1};
}

Point MyPlayer::find_gap_block(const State& state, Sign opponent) const {
  auto candidates = get_candidate_moves(state);
  
  for (const auto& c : candidates) {
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (auto& d : dirs) {
      for (int offset = -4; offset <= 0; ++offset) {
        int opponent_count = 0;
        int empty_count = 0;
        bool valid = true;
        
        for (int step = 0; step < 5; ++step) {
          int nx = c.x + d[0] * (offset + step);
          int ny = c.y + d[1] * (offset + step);
          
          if (nx < 0 || nx >= state.get_opts().cols || 
              ny < 0 || ny >= state.get_opts().rows) {
            valid = false;
            break;
          }
          
          Sign s = state.get_value(nx, ny);
          if (s == opponent) {
            opponent_count++;
          } else if (s == Sign::NONE) {
            empty_count++;
          } else if (s != m_sign) {
            valid = false;
            break;
          }
        }
        
        if (!valid) continue;
        
        if (opponent_count >= 3 && empty_count >= 2) 
          return c;        
      }
    }
  }
  
  return {-1, -1};
}

std::vector<Point> MyPlayer::get_candidate_moves(const State &state) const {
  std::vector<Point> moves;

  int rows = state.get_opts().rows;
  int cols = state.get_opts().cols;
  int cx = cols / 2;
  int cy = rows / 2;

  int total_free = count_free_cells(state);

  if (total_free > 300) {
    for (int r = 0; r < 5 && moves.size() < 20; ++r) {
      for (int dx = -r; dx <= r; ++dx) {
        for (int dy = -r; dy <= r; ++dy) {
          if (abs(dx) == r || abs(dy) == r) {
            int x = cx + dx;
            int y = cy + dy;

            if (x >= 0 && x < cols && y >= 0 && y < rows) {
              if (state.get_value(x, y) == Sign::NONE) {
                moves.push_back({x, y});
              }
            }
          }
        }
      }
    }
    return moves;
  }
    
  for (int y = 0; y < rows && moves.size() < 60; ++y) {
    for (int x = 0; x < cols && moves.size() < 60; ++x) {
      if (state.get_value(x, y) != Sign::NONE) continue;
      
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
    for (int y = 0; y < rows; ++y) {
      for (int x = 0; x < cols; ++x) {
        if (state.get_value(x, y) == Sign::NONE) {
          moves.push_back({x, y});
          break;
        }
      }
      if (!moves.empty()) break;
    }
  }

  return moves; 
}

// std::vector<Point> MyPlayer::get_local_candidate(const State &state, 
//   int local_x, int local_y, int radius) const {

//   std::vector<Point> moves;

//   int rows = state.get_opts().rows;
//   int cols = state.get_opts().cols;
//   int cx = cols / 2;
//   int cy = rows / 2;

//   for (int dx = -radius; dx <= radius; ++dx) {
//     for (int dy = -radius; dy <= radius; ++dy) {
//       if (dx == 0 && dy == 0) continue;

//       int x = local_x + dx;
//       int y = local_y + dy;

//       if (!square_is_free(state, x, y)) continue;
//       if (has_neighbors(state, x, y, 2))
//         moves.push_back({x, y});
//     }  
//   }

//   std::sort(moves.begin(), moves.end(), [cx, cy](const Point & a, const Point& b) {
//     return std::abs(a.x - cx) + std::abs(a.y - cy) < 
//           std::abs(b.x - cx) + std::abs(b.y - cy);
//   });

//   return moves;
// }

void MyPlayer::init_sim_board(const State &state) const {
  m_sim_rows = state.get_opts().rows;
  m_sim_cols = state.get_opts().cols;

  for (int y = 0; y < m_sim_rows; ++y) {
    for (int x = 0; x < m_sim_cols; ++x) {
      m_sim_board[x + y * m_sim_cols] = state.get_value(x, y);
    }
  }
}

void MyPlayer::sim_place_sign(int x, int y, Sign sign) const {
  if (x >= 0 && x < m_sim_cols && y >= 0 && y < m_sim_rows) 
    m_sim_board[x + y * m_sim_cols] = sign;
}

void MyPlayer::sim_clear(int x, int y) const {
  if (x >= 0 && x < m_sim_cols && y >= 0 && y < m_sim_rows) 
    m_sim_board[x + y * m_sim_cols] = Sign::NONE;
}

Sign MyPlayer::sim_get(int x, int y) const {
  if (x < 0 || x >= m_sim_cols || y < 0 || y >= m_sim_rows) 
        return Sign::WALL;
    
  return m_sim_board[x + y * m_sim_cols];
}

bool MyPlayer::would_win_sim(Sign player) const {
  const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};

  for (int y = 0; y < m_sim_rows; ++y) {
    for (int x = 0; x < m_sim_cols; ++x) {
      if (sim_get(x, y) != player) continue;

      for (auto& d : dirs) {
        int count = 1;
        for (int step = 1; step < 5; ++step) {
          if (sim_get(x + d[0]*step, y + d[1]*step) == player) count++;
          else break;
        }
        for (int step = 1; step < 5; ++step) {
          if (sim_get(x - d[0]*step, y - d[1]*step) == player) count++;
          else break;
        }

        if (count >= 5) return true;
      }
    }
  }
  return false;
}

Point MyPlayer::find_endgame_with_sim(const State& state, Sign opponent) const {
  auto candidates = get_candidate_moves(state);
  if (candidates.empty()) return {-1, -1};
  
  init_sim_board(state);
  
  Point best = candidates[0];
  int best_score = -1000000;
  
  int max_check = std::min(20, (int)candidates.size());
  
  for (int i = 0; i < max_check; ++i) {
    const auto& c = candidates[i];
   
    sim_place_sign(c.x, c.y, m_sign);
    
    int score = 0;
    
    if (would_win_sim(m_sign)) {
      sim_clear(c.x, c.y);
      return c;
    }
   
    int threats = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
      int len = 1;
      for (int step = 1; step < 5; ++step) {
        if (sim_get(c.x + d[0]*step, c.y + d[1]*step) == m_sign) len++;
        else break;
      }
      for (int step = 1; step < 5; ++step) {
        if (sim_get(c.x - d[0]*step, c.y - d[1]*step) == m_sign) len++;
        else break;
      }
      if (len >= 3) threats++;
    }
    score += threats * 500;
  
    int opponent_threats = 0;
    for (int dy = -2; dy <= 2; ++dy) {
      for (int dx = -2; dx <= 2; ++dx) {
        int nx = c.x + dx, ny = c.y + dy;
        if (nx >= 0 && nx < m_sim_cols && ny >= 0 && ny < m_sim_rows) {
          if (sim_get(nx, ny) == Sign::NONE) {
            sim_place_sign(nx, ny, opponent);
            if (would_win_sim(opponent)) {
              opponent_threats += 100;
            }
            sim_clear(nx, ny);
          }
        }
      }
    }
    score -= opponent_threats;
   
    int cx = state.get_opts().cols / 2;
    int cy = state.get_opts().rows / 2;
    int dist = abs(c.x - cx) + abs(c.y - cy);
    score += (40 - dist) * 3;
    
    if (score > best_score) {
      best_score = score;
      best = c;
    }
    
    sim_clear(c.x, c.y);
  }
  
  return best;
}

Point MyPlayer::find_strategic_block(const State& state, Sign opponent) const {
  auto candidates = get_candidate_moves(state);
  int max_check = std::min(15, (int)candidates.size());
  
  for (int i = 0; i < max_check; ++i) {
    const auto& c = candidates[i];
    auto seq = SequencesAnalyzer::analyze(state, c.x, c.y, 5);
    const Pattern* dirs[4] = {&seq.hor, &seq.ver, &seq.diag_rd, &seq.diag_ld};
    for (const auto* p : dirs) {
      if (p->sign == opponent && p->length == 3 && p->open_ends == 2)
        return c;
    }
  }
  return {-1, -1};
}

Point MyPlayer::find_best_by_heuristic(const State& state, Sign opponent) const {
  auto candidates = get_candidate_moves(state);
  if (candidates.empty()) return {-1, -1};
    
  struct ScoredPoint {
    Point p;
    int fast_score;
  };
    
  std::vector<ScoredPoint> scored;
  scored.reserve(candidates.size());
    
  int cx = state.get_opts().cols / 2;
  int cy = state.get_opts().rows / 2;
  const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
  for (const auto& c : candidates) {
    int fast_score = 0;
        
    for (auto& d : dirs) { 
      Pattern my_pat = SequencesAnalyzer::scan_direction(state, 
        c.x, c.y, d[0], d[1], m_sign, 5);
      Pattern opp_pat = SequencesAnalyzer::scan_direction(state, 
        c.x, c.y, d[0], d[1], opponent, 5);

      fast_score += my_pat.get_pattern_weight() * ATTACK_MULTIPLIER;
            
      fast_score += opp_pat.get_pattern_weight() * DEFENSE_MULTIPLIER;
    }
    
    int threats_created = 0;
  for (auto& d : dirs) {
    int my_len = 1;
    for (int step = 1; step < 5; ++step) {
        Sign s = state.get_value(c.x + d[0]*step, c.y + d[1]*step);
        if (s == m_sign) my_len++;
        else break;
    }
    for (int step = 1; step < 5; ++step) {
        Sign s = state.get_value(c.x - d[0]*step, c.y - d[1]*step);
        if (s == m_sign) my_len++;
        else break;
    }
    if (my_len >= 3) threats_created++;
}
// Если ход создает 2+ угрозы (вилка) - огромный бонус!
  if (threats_created >= 2) {
    fast_score += 50000;  // Бонус за вилку!
}

    int dist = abs(c.x - cx) + abs(c.y - cy);
    fast_score += WEIGHT_CENTER * (20 - dist);
        
    if (has_neighbors(state, c.x, c.y, 1)) fast_score += WEIGHT_NEIGHBOR * 3;
    else if (has_neighbors(state, c.x, c.y, 2)) fast_score += WEIGHT_NEIGHBOR;
        
    scored.push_back({c, fast_score});
  }
    
  std::sort(scored.begin(), scored.end(),
    [](const ScoredPoint& a, const ScoredPoint& b) {
      return a.fast_score > b.fast_score;
    });
    
  int top_n = std::min(5, (int)scored.size());
  Point best = scored[0].p;
  int best_score = -1000000;
    
  for (int i = 0; i < top_n; ++i) {
    const auto& c = scored[i].p;
    auto seq = SequencesAnalyzer::analyze(state, c.x, c.y, 5);
    int score = seq.score_for(m_sign, 5) + seq.score_for(opponent, 5);
        
    int dist = abs(c.x - cx) + abs(c.y - cy);
    score += WEIGHT_CENTER * (20 - dist);
        
    if (score > best_score) {
      best_score = score;
      best = c;
    }
  }
    
    return best;
}

}; // namespace ttt::my_player