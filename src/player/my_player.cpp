#include "my_player.hpp"
#include <cstdlib>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
  Sign opponent = (m_sign == Sign::X) ? Sign::O : Sign::X;

  auto candidates = get_candidate_moves(state);
  if (!candidates.empty()) 
    return {0, 0};

  for (const auto &c : candidates) {
    if (would_win(state, c.x, c.y)) return c;
  }

  auto immediate_threats = SequencesAnalyzer::find_all_threats(state, opponent, 5);
  if (!immediate_threats.empty()) {
    return immediate_threats[0];
  }

  for (const auto &c : candidates) {
    auto analysis = SequencesAnalyzer::analyze(state, c.x, c.y, 5);
    const Pattern* dirs[4] = {&analysis.hor, &analysis.ver, &analysis.diag_rd, &analysis.diag_ld};
        
    for (const auto* pat : dirs) {
      if (pat->sign == opponent && pat->length == 3 && pat->open_ends == 2) 
        return c; 
    }
  }
  
  return candidates[0];
}

// ==================================================================

bool MyPlayer::square_is_free(const State &state, int x, int y) const {
  return in_bounds(state, x, y) && state.get_value(x, y) == Sign::NONE;
}

bool MyPlayer::in_bounds(const State &state, int x, int y) const {
  return x >= 0 && x < state.get_opts().cols && y >= 0 && y < state.get_opts().rows;
}

bool MyPlayer::has_neighbors(const State &state, int x, int y, int radius) const {  
  for (int dx = -radius; dx <= radius; ++dx) {
      for (int dy = -radius; dy <= radius; ++dy) {
        if (dx == 0 && dy == 0) continue;

        int nx = x + dx;
        int ny = y + dy;

        if (in_bounds(state, nx, ny)) {
          const Sign val = state.get_value(nx, ny);

          if (val == Sign::X || val == Sign::O) {
            return true;
          }
        }        
      }
    }
    return false;
}

Point MyPlayer::find_best_start(const State &state) const {
  int cx = state.get_opts().cols / 2;
  int cy = state.get_opts().rows / 2;

  int max_radius = std::max({cx, state.get_opts().cols - 1 - cx, 
    cy, state.get_opts().rows - 1 - cy});

  for (int r = 0; r <= max_radius; ++r) {
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

std::vector<Point> MyPlayer::get_candidate_moves(const State &state) const {
  std::vector<Point> moves;

  int rows = state.get_opts().rows;
  int cols = state.get_opts().cols;
  int cx = cols / 2;
  int cy = rows / 2;

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      if (!square_is_free(state, x, y)) continue;

      if (has_neighbors(state, x, y)) 
        moves.push_back({x, y}); //агрегация
    }
  }

  if (moves.empty()) {
    Point best = find_best_start(state);
    if (square_is_free(state, best.x, best.y))
      moves.push_back(best); 
  }

  std::sort(moves.begin(), moves.end(), [cx, cy](const Point & a, const Point& b) {
    return std::abs(a.x - cx) + std::abs(a.y - cy) < 
          std::abs(b.x - cx) + std::abs(b.y - cy);
  });

  return moves; 
}

std::vector<Point> MyPlayer::get_local_candidate(const State &state, 
  int local_x, int local_y, int radius) const {

  std::vector<Point> moves;

  int rows = state.get_opts().rows;
  int cols = state.get_opts().cols;
  int cx = cols / 2;
  int cy = rows / 2;

  for (int dx = -radius; dx <= radius; ++dx) {
    for (int dy = -radius; dy <= radius; ++dy) {
      if (dx == 0 && dy == 0) continue;

      int x = local_x + dx;
      int y = local_y + dy;

      if (!square_is_free(state, x, y)) continue;
      if (has_neighbors(state, x, y, 2))
        moves.push_back({x, y});
    }  
  }

  std::sort(moves.begin(), moves.end(), [cx, cy](const Point & a, const Point& b) {
    return std::abs(a.x - cx) + std::abs(a.y - cy) < 
          std::abs(b.x - cx) + std::abs(b.y - cy);
  });

  return moves;
}

void MyPlayer::init_sim_board(const State &state) {
  m_sim_rows = state.get_opts().rows;
  m_sim_cols = state.get_opts().cols;

  for (int y = 0; y < m_sim_rows; ++y) {
    for (int x = 0; x < m_sim_cols; ++x) {
      m_sim_board[x + y * m_sim_cols] = state.get_value(x, y);
    }
  }
}

void MyPlayer::sim_place_sign(int x, int y, Sign sign) {
  if (x >= 0 && x < m_sim_cols && y >= 0 && y < m_sim_rows) 
    m_sim_board[x + y * m_sim_cols] = sign;
}

void MyPlayer::sim_clear(int x, int y) {
  if (x >= 0 && x < m_sim_cols && y >= 0 && y < m_sim_rows) 
    m_sim_board[x + y * m_sim_cols] = Sign::NONE;
}

Sign MyPlayer::sim_get(int x, int y) const {
  if (x < 0 || x >= m_sim_cols || y < 0 || y >= m_sim_rows) 
        return Sign::WALL;
    
  return m_sim_board[x + y * m_sim_cols];
}

bool MyPlayer::would_win(const State &state, int x, int y) {
  init_sim_board(state);
  sim_place_sign(x, y, m_sign);
    
  static const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
  for (auto &d : dirs) {
      int count = 1; 
      for (int step = 1; step < 5; ++step)
          if (sim_get(x + d[0]*step, y + d[1]*step) == m_sign) ++count; else break;
      for (int step = 1; step < 5; ++step)
          if (sim_get(x - d[0]*step, y - d[1]*step) == m_sign) ++count; else break;
      if (count >= 5) return true;
  }
  return false;
}

}; // namespace ttt::my_player
