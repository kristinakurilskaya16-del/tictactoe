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

  // счётчик вызовов negamax
  mutable int m_nodes_visited = 0;   
  static constexpr int MAX_NODES = 200; 

  // вспомогательные методы
  bool square_is_free(const State &state, int x, int y) const;
  bool in_bounds(const State &state, int x, int y) const;
  int count_free_cells(const State& state) const;

  // лучший старт
  Point find_best_start(const State &state) const;

  // возможные ходы
  std::vector<Point> get_candidate_moves(const State &state) const;
  std::vector<Point> order_candidate_moves(const State& state, Sign player,
                                            int max_moves) const;

  // проверка выигрыша
  bool would_win_fast(const State& state, int x, int y, Sign player) const;

  //быстрая оценка всей доски
  int evaluate_board_window(const State& state, Sign player) const;
  int line_value(int count) const;

  int negamax(const State& state, int depth, int alpha, int beta,
    Sign current_player, int last_x = -1, int last_y = -1) const;
    
    inline Sign safe_get(const State& state, int x, int y) const {
        if (x < 0 || y < 0 ||
            x >= state.get_opts().cols ||
            y >= state.get_opts().rows)
            return Sign::WALL;

        return state.get_value(x, y);
    }

public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

struct ScoredMove{
    Point move;
    int score;
};

}; // namespace ttt::my_player
