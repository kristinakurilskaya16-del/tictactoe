#include <gtest/gtest.h>
#include "player/my_player.hpp"
#include "core/game.hpp"

using namespace ttt::my_player;
using namespace ttt::game;

class MyPlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        State::Opts opts;
        opts.rows = 20;
        opts.cols = 20;
        opts.win_len = 5;
        opts.max_moves = 400;
        state = new State(opts);
        player = new MyPlayer("TestPlayer");
        player->set_sign(Sign::X);
    }

    void TearDown() override {
        delete state;
        delete player;
    }

    State* state;
    MyPlayer* player;
};

// Тест 1: AI делает ход в пределах доски
TEST_F(MyPlayerTest, MakesValidMove) {
    Point move = player->make_move(*state);
    EXPECT_GE(move.x, 0);
    EXPECT_LT(move.x, 20);
    EXPECT_GE(move.y, 0);
    EXPECT_LT(move.y, 20);
}

// Тест 2: AI не ходит в занятые клетки
TEST_F(MyPlayerTest, AvoidsOccupied) {
    for (int i = 0; i < 5; i++) {
        Point move = player->make_move(*state);
        EXPECT_EQ(state->get_value(move.x, move.y), Sign::NONE);
        state->process_move(Sign::X, move.x, move.y);
    }
}

// Тест 3: AI работает с разными знаками
TEST_F(MyPlayerTest, WorksWithDifferentSign) {
    player->set_sign(Sign::O);
    Point move = player->make_move(*state);
    EXPECT_GE(move.x, 0);
    EXPECT_LT(move.x, 20);
    EXPECT_GE(move.y, 0);
    EXPECT_LT(move.y, 20);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}