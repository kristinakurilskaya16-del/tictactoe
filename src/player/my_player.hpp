#pragma once

#include "core/game.hpp"
#include "sequences.hpp"
#include <array>
#include <vector>

namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

class MyPlayer : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;

  // ===========================================================

  int negamax(const State& state, int depth, int alpha, int beta,
            Sign current_player, int last_x, int last_y) const;

// Простая оценка позиции (использует существующие веса)
  int evaluate_position_simple(const State& state, Sign player) const;

// Получение ограниченного списка ходов
  std::vector<Point> get_top_moves_simple(const State& state, Sign player, int max_moves) const;

  bool square_is_free(const State &state, int x, int y) const;
  bool in_bounds(const State &state, int x, int y) const;
  bool has_neighbors(const State &state, int x, int y, int radius = 2) const;
  int count_free_cells(const State& state) const;
  Point find_best_start(const State &state) const;

  std::vector<Point> get_candidate_moves(const State &state) const;
  std::vector<Point> get_local_candidate(const State &state, int local_x, 
                                        int local_y, int radius = 3) const;

  mutable std::array<Sign, 400> m_sim_board;
  mutable int m_sim_cols = 20;
  mutable int m_sim_rows = 20;

  void init_sim_board(const State &state) const;
  void sim_place_sign(int x, int y, Sign sign) const;
  void sim_clear(int x, int y) const;
  Sign sim_get(int x, int y) const;

  bool would_win_fast(const State& state, int x, int y, Sign player) const;
  Point find_immediate_win_fast(const State& state) const;
  Point find_immediate_block_fast(const State& state, Sign opponent) const;
  
  Point find_gap_block(const State& state, Sign opponent) const;

  Point find_strategic_block(const State& state, Sign opponent) const;
  Point find_best_by_heuristic(const State& state, Sign opponent) const;
  Point find_endgame_with_sim(const State& state, Sign opponent) const;
  
  bool would_win_sim(Sign player) const;

public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

}; // namespace ttt::my_player