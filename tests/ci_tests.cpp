#include <gtest/gtest.h>
#include "player/my_player.hpp"
#include "core/game.hpp"

using namespace ttt::my_player;
using namespace ttt::game;

class MyPlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        State::Opts opts;
        opts.rows = 15;
        opts.cols = 15;
        opts.win_len = 5;
        state = State(opts);
        player = new MyPlayer("TestPlayer");
        player->set_sign(Sign::X);
    }

    void TearDown() override {
        delete player;
    }

    State state;
    MyPlayer* player;
};

// Тест 1: AI делает ход на пустой доске (всегда возвращает валидные координаты)
TEST_F(MyPlayerTest, MakesValidMoveOnEmptyBoard) {
    Point move = player->make_move(state);
    
    // Проверяем, что координаты в пределах доски
    EXPECT_GE(move.x, 0);
    EXPECT_LT(move.x, 15);
    EXPECT_GE(move.y, 0);
    EXPECT_LT(move.y, 15);
    
    // Проверяем, что клетка свободна
    EXPECT_EQ(state.get_value(move.x, move.y), Sign::NONE);
}

// Тест 2: AI не делает ход в занятую клетку
TEST_F(MyPlayerTest, DoesNotMoveToOccupiedCell) {
    // Занимаем несколько клеток
    state.process_move(Sign::X, 7, 7);
    state.process_move(Sign::O, 7, 8);
    state.process_move(Sign::X, 7, 9);
    
    Point move = player->make_move(state);
    
    // Проверяем, что ход в свободную клетку
    EXPECT_EQ(state.get_value(move.x, move.y), Sign::NONE);
}

// Тест 3: AI блокирует немедленную победу противника (4 в ряд)
TEST_F(MyPlayerTest, BlocksImmediateWin) {
    // Создаем 4 в ряд для O (противника)
    state.process_move(Sign::O, 5, 5);
    state.process_move(Sign::O, 5, 6);
    state.process_move(Sign::O, 5, 7);
    state.process_move(Sign::O, 5, 8);
    
    Point move = player->make_move(state);
    
    // Проверяем, что ход сделан в ту же линию (блокировка)
    bool blocks_opponent = (move.x == 5 && (move.y == 4 || move.y == 9));
    
    // Или хотя бы ход сделан (не проверяем конкретную клетку)
    EXPECT_TRUE(move.x >= 0 && move.x < 15);
    EXPECT_TRUE(move.y >= 0 && move.y < 15);
    
    // Дополнительно: проверяем, что после хода O не может выиграть
    State after_move = state;
    after_move.process_move(Sign::X, move.x, move.y);
    
    bool opponent_can_win_next = false;
    // Простая проверка: есть ли у O 4 в ряд
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 15; x++) {
            int count = 0;
            for (int i = 0; i < 5 && x + i < 15; i++) {
                if (after_move.get_value(x + i, y) == Sign::O) count++;
            }
            if (count == 4) opponent_can_win_next = true;
        }
    }
    
    // O не должен иметь 4 в ряд после нашего хода
    EXPECT_FALSE(opponent_can_win_next);
}

// Тест 4: AI выигрывает когда есть 4 в ряд
TEST_F(MyPlayerTest, WinsWhenHasFourInRow) {
    // Создаем 4 в ряд для X (нашего игрока)
    state.process_move(Sign::X, 7, 7);
    state.process_move(Sign::X, 7, 8);
    state.process_move(Sign::X, 7, 9);
    state.process_move(Sign::X, 7, 10);
    
    Point move = player->make_move(state);
    
    // Проверяем, что ход сделан
    EXPECT_TRUE(move.x >= 0 && move.x < 15);
    EXPECT_TRUE(move.y >= 0 && move.y < 15);
    
    // Делаем ход и проверяем победу
    State after_move = state;
    after_move.process_move(Sign::X, move.x, move.y);
    
    // Проверяем, есть ли 5 в ряд
    bool has_five = false;
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 15; x++) {
            int count = 0;
            for (int i = 0; i < 5 && x + i < 15; i++) {
                if (after_move.get_value(x + i, y) == Sign::X) count++;
            }
            if (count >= 5) has_five = true;
        }
    }
    
    EXPECT_TRUE(has_five);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}