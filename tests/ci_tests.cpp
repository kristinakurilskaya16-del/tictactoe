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
        
        // Создаем State в куче (через new)
        p_state = new State(opts);
        player = new MyPlayer("TestPlayer");
        player->set_sign(Sign::X);
    }

    void TearDown() override {
        delete p_state;
        delete player;
    }

    State* p_state;  // Указатель на State
    MyPlayer* player;
};

// Тест 1: Проверка базовых выигрышных комбинаций
TEST_F(MyPlayerTest, DetectsImmediateWin) {
    // Создаем ситуацию с 4 в ряд для X
    p_state->process_move(Sign::X, 7, 7);
    p_state->process_move(Sign::X, 7, 8);
    p_state->process_move(Sign::X, 7, 9);
    p_state->process_move(Sign::X, 7, 10);
    
    Point move = player->make_move(*p_state);
    
    // Игрок должен поставить 5-й камень для победы
    EXPECT_EQ(move.x, 7);
    EXPECT_TRUE(move.y == 6 || move.y == 11);
    EXPECT_TRUE(p_state->get_value(move.x, move.y) == Sign::NONE);
}

// Тест 2: Проверка блокировки выигрыша противника
TEST_F(MyPlayerTest, BlocksOpponentWin) {
    // Противник (O) имеет 4 в ряд
    p_state->process_move(Sign::O, 5, 5);
    p_state->process_move(Sign::O, 5, 6);
    p_state->process_move(Sign::O, 5, 7);
    p_state->process_move(Sign::O, 5, 8);
    
    Point move = player->make_move(*p_state);
    
    // Должен заблокировать 5-й камень противника
    EXPECT_EQ(move.x, 5);
    EXPECT_TRUE(move.y == 4 || move.y == 9);
}

// Тест 3: Проверка защиты от немедленного проигрыша
TEST_F(MyPlayerTest, PrioritizesDefenseOverAttack) {
    // Создаем ситуацию: у X есть 3 в ряд, у O есть 4 в ряд
    p_state->process_move(Sign::X, 10, 10);
    p_state->process_move(Sign::X, 10, 11);
    p_state->process_move(Sign::X, 10, 12);
    
    p_state->process_move(Sign::O, 5, 5);
    p_state->process_move(Sign::O, 5, 6);
    p_state->process_move(Sign::O, 5, 7);
    p_state->process_move(Sign::O, 5, 8);
    
    Point move = player->make_move(*p_state);
    
    // Должен заблокировать победу O, а не создавать свою
    EXPECT_EQ(move.x, 5);
    EXPECT_TRUE(move.y == 4 || move.y == 9);
}

// Тест 4: Проверка первого хода (должен быть в центр)
TEST_F(MyPlayerTest, FirstMoveNearCenter) {
    Point move = player->make_move(*p_state);
    
    // Первый ход должен быть в центре или рядом
    int center_x = 9;  // для поля 20x20 центр в 9-10
    int center_y = 9;
    int dx = abs(move.x - center_x);
    int dy = abs(move.y - center_y);
    
    EXPECT_LE(dx + dy, 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}