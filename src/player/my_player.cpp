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
    
    // 2. Критическая блокировка
    auto candidates = get_candidate_moves(state);
    for (const auto& c : candidates) {
        if (would_win_fast(state, c.x, c.y, opponent)) 
            return c;
    }
    
    // 3. Первые ходы - в центр
    if (state.get_move_no() < 3) {
        Point start = find_best_start(state);
        if (start.x >= 0) return start;
    }
    
    // 4. Определяем глубину в зависимости от стадии игры
    int free_cells = count_free_cells(state);

    int depth;
    if (free_cells > 250) depth = 2;
    else if (free_cells > 120) depth = 4;
    else depth = 6;
    
    // 5. Получаем топ ходы для поиска
    std::vector<Point> top_moves = get_top_moves_simple(state, m_sign, 8);
    if (top_moves.empty()) {
        for (int y = 0; y < state.get_opts().rows; ++y)
            for (int x = 0; x < state.get_opts().cols; ++x)
                if (square_is_free(state, x, y)) return {x, y};
        return {0, 0};
    }
    
    // 6. Negamax поиск
    Point best_move = top_moves[0];
    int best_value = -1000000000;
    int alpha = -1000000000;
    int beta = 1000000000;
    
    for (const auto& move : top_moves) {
        State new_state = state;
        new_state.process_move(m_sign, move.x, move.y);
        
        int value = -negamax(new_state, depth - 1, -beta, -alpha,
                             opponent, move.x, move.y);
        
        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
        
        alpha = std::max(alpha, value);
        if (alpha >= beta) break;
    }
    
    return best_move;
}

// ==================================================================

int MyPlayer::evaluate_position_simple(const State& state, Sign player) const {
    Sign opponent = (player == Sign::X) ? Sign::O : Sign::X;
    int score = 0;
    
    // Считаем ВСЕ пустые клетки с соседями (а не только свои фигуры)
    for (int x = 0; x < state.get_opts().cols; ++x) {
        for (int y = 0; y < state.get_opts().rows; ++y) {
            if (state.get_value(x, y) != Sign::NONE) continue;
            if (!has_neighbors(state, x, y, 2)) continue;
            
            const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
            for (auto& d : dirs) {
                // СВОИ фигуры (атака) - большой вес
                int my_len = 1;
                for (int step = 1; step < 5; ++step) {
                    Sign s = state.get_value(x + d[0]*step, y + d[1]*step);
                    if (s == player) my_len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                for (int step = 1; step < 5; ++step) {
                    Sign s = state.get_value(x - d[0]*step, y - d[1]*step);
                    if (s == player) my_len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                
                // ФИГУРЫ ПРОТИВНИКА (защита) - вес чуть меньше
                int opp_len = 1;
                for (int step = 1; step < 5; ++step) {
                    Sign s = state.get_value(x + d[0]*step, y + d[1]*step);
                    if (s == opponent) opp_len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                for (int step = 1; step < 5; ++step) {
                    Sign s = state.get_value(x - d[0]*step, y - d[1]*step);
                    if (s == opponent) opp_len++;
                    else if (s == Sign::NONE) break;
                    else break;
                }
                
                // КРИТИЧЕСКИ ВАЖНЫЕ ВЕСА (из вашей weight.hpp)
                // Свои фигуры
                if (my_len >= 5) score += 10000000;
                else if (my_len == 4) score += 100000;
                else if (my_len == 3) score += 30000;
                else if (my_len == 2) score += 2000;
                else if (my_len == 1) score += 50;
                
                // Фигуры противника (угрозы, которые нужно блокировать)
                if (opp_len >= 5) score -= 10000000;
                else if (opp_len == 4) score -= 200000;  // ВАЖНО! 4 противника
                else if (opp_len == 3) score -= 30000;
                else if (opp_len == 2) score -= 2000;
            }
        }
    }
    
    // Бонус за центр (для всех фигур)
    int cx = state.get_opts().cols / 2;
    int cy = state.get_opts().rows / 2;
    for (int x = 0; x < state.get_opts().cols; ++x) {
        for (int y = 0; y < state.get_opts().rows; ++y) {
            if (state.get_value(x, y) == player) {
                int dist = abs(x - cx) + abs(y - cy);
                score += (20 - dist) * 5;
            }
        }
    }
    
    return score;
}

// ==================================================================
// Получение топ ходов (ограниченное количество)
// ==================================================================

std::vector<Point> MyPlayer::get_top_moves_simple(const State& state, Sign player, int max_moves) const {
    Sign opponent = (player == Sign::X) ? Sign::O : Sign::X;
    std::vector<std::pair<Point, int>> scored;
    
    auto candidates = get_candidate_moves(state);
    if (candidates.empty()) return {};
    
    for (const auto& move : candidates) {
        int score = 0;
        const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
        
        for (auto& d : dirs) {
            // Свои фигуры
            int my_len = 1;
            for (int step = 1; step < 5; ++step) {
                Sign s = state.get_value(move.x + d[0]*step, move.y + d[1]*step);
                if (s == player) my_len++;
                else break;
            }
            for (int step = 1; step < 5; ++step) {
                Sign s = state.get_value(move.x - d[0]*step, move.y - d[1]*step);
                if (s == player) my_len++;
                else break;
            }
            
            // Фигуры противника
            int opp_len = 1;
            for (int step = 1; step < 5; ++step) {
                Sign s = state.get_value(move.x + d[0]*step, move.y + d[1]*step);
                if (s == opponent) opp_len++;
                else break;
            }
            for (int step = 1; step < 5; ++step) {
                Sign s = state.get_value(move.x - d[0]*step, move.y - d[1]*step);
                if (s == opponent) opp_len++;
                else break;
            }
            
            // Атака важнее защиты (коэффициенты из weight.hpp)
            if (my_len >= 5) score += 10000000;
            else if (my_len == 4) score += 100000;
            else if (my_len == 3) score += 30000;
            else if (my_len == 2) score += 2000;
            else if (my_len == 1) score += 50;
            
            if (opp_len >= 4) score += 80000;  // Критическая блокировка
            else if (opp_len == 3) score += 15000;
            else if (opp_len == 2) score += 1000;
        }
        
        // Бонус за центр
        int cx = state.get_opts().cols / 2;
        int cy = state.get_opts().rows / 2;
        int dist = abs(move.x - cx) + abs(move.y - cy);
        score += (20 - dist) * 10;
        
        scored.push_back({move, score});
    }
    
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<Point> result;
    for (int i = 0; i < std::min(max_moves, (int)scored.size()); ++i) {
        result.push_back(scored[i].first);
    }
    
    return result;
}

// ==================================================================
// Negamax с альфа-бета отсечением
// ==================================================================

int MyPlayer::negamax(const State& state, int depth, int alpha, int beta,
                      Sign current_player, int last_x, int last_y) const {
  Sign prev_player = (current_player == Sign::X) ? Sign::O : Sign::X;
  Sign next_player = prev_player;

// Проверяем: предыдущий игрок сделал last move
  if (last_x >= 0 && last_y >= 0) {
    if (would_win_fast(state, last_x, last_y, prev_player)) 
      return -10000000 + depth;
  }
    
     // Достигли максимальной глубины - оцениваем позицию
    if (depth == 0) {
        return evaluate_position_simple(state, m_sign);
    }
    
    // Получаем ограниченный список ходов (топ-6 для скорости)
    std::vector<Point> moves = get_top_moves_simple(state, current_player, 6);
    if (moves.empty()) return evaluate_position_simple(state, m_sign);
    
    int best_value = -1000000000;
    
    for (const auto& move : moves) {
        // Создаем новое состояние (копирование, но с ограниченными ходами терпимо)
        State new_state = state;
        new_state.process_move(current_player, move.x, move.y);
        
        int value = -negamax(new_state, depth - 1, -beta, -alpha,
                             next_player, move.x, move.y);
        
        if (value > best_value) {
            best_value = value;
        }
        
        alpha = std::max(alpha, value);
        if (alpha >= beta) {
            break;  // Alpha-beta отсечение
        }
    }
    
    return best_value;
}

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