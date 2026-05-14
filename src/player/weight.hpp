#pragma once

namespace ttt::my_player {

// Победа
constexpr int WEIGHT_WIN = 100'000'000; // XXXXX - пять в ряд 

// Четвёрки (сплошные)
constexpr int WEIGHT_OPEN_FOUR = 1'000'000; // _XXXX_ - четыре в ряд (open_ends=2)
constexpr int WEIGHT_HALF_FOUR = 300'000; // _XXXX - четыре в ряд (open_ends=1)
constexpr int WEIGHT_CLOSED_FOUR = 0; // XXXX - четыре в ряд (open_ends=0, бесполезна)

// Четвёрки с дырой
constexpr int WEIGHT_GAP_FOUR_OPEN2  = 250'000; // _X_XXX_ и т.п. – четыре в ряд (open_ends=2)
constexpr int WEIGHT_GAP_FOUR_OPEN1  = 180'000; // _X_XXX  или X_XXX_ – четыре в ряд (open_ends=1)
constexpr int WEIGHT_GAP_FOUR_OPEN0  = 120'000; // X_XXX – четыре в ряд (open_ends=0)

// Тройки (сплошные)
constexpr int WEIGHT_OPEN_THREE = 30'000; // _XXX_ - три в ряд (open_ends=2)
constexpr int WEIGHT_HALF_THREE = 5'000; // _XXX - три в ряд (open_ends=1)
constexpr int WEIGHT_CLOSED_THREE = 0; // XXX - три в ряд (open_ends=0, бесполезна)

// Тройки с дырой
constexpr int WEIGHT_GAP_THREE_OPEN2 = 20'000; // _XX_X_ и т.п.
constexpr int WEIGHT_GAP_THREE_OPEN1 = 15'000; // _XX_X  или XX_X_
constexpr int WEIGHT_GAP_THREE_OPEN0 = 0; // XX_X

// Двойки (сплошные)
constexpr int WEIGHT_OPEN_TWO = 5'000; // _XX_ - два в ряд (open_ends=2)
constexpr int WEIGHT_HALF_TWO = 1'000; // _XX - два в ряд (open_ends=1)
constexpr int WEIGHT_CLOSED_TWO = 0; // XX - два в ряд (open_ends=0, бесполезна)

// Двойки с дырой
constexpr int WEIGHT_GAP_TWO_OPEN2   = 4'000;     // _X_X_
constexpr int WEIGHT_GAP_TWO_OPEN1   = 2'000;     // _X_X  или X_X_
constexpr int WEIGHT_GAP_TWO_OPEN0   = 0;      // X_X

// Одиночный камень
constexpr int WEIGHT_SINGLE = 50; // один камень, позиционный вес

// Бонус за центр 
constexpr int WEIGHT_CENTER = 40;

// Множители для атаки/защиты
constexpr double ATTACK_FACTOR = 1.0;
constexpr double DEFENSE_FACTOR = 0.9; // защита на 10% слабее атаки

} // namespace ttt::my_player