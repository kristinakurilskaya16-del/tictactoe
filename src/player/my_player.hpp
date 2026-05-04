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

  bool square_is_free(const State &state, int x, int y) const;
  bool in_bounds(const State &state, int x, int y) const;
  bool has_neighbors(const State &state, int x, int y, int radius = 2) const;
  Point find_best_start(const State &state) const;

  std::vector<Point> get_candidate_moves(const State &state) const;
  std::vector<Point> get_local_candidate(const State &state, int local_x, 
                                        int local_y, int radius = 3) const;

  std::array<Sign, 400> m_sim_board;
  int m_sim_cols = 20;
  int m_sim_rows = 20;

  void init_sim_board(const State &state);
  void sim_place_sign(int x, int y, Sign sign);
  void sim_clear(int x, int y);
  Sign sim_get(int x, int y) const;

  Point find_immediate_win(const State& state);
  Point find_immediate_block(const State& state, Sign opponent);
  Point find_strategic_block(const State& state, Sign opponent);
  Point find_best_by_heuristic(const State& state, Sign opponent);
  bool would_win(const State &state, int x, int y);

public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

}; // namespace ttt::my_player
