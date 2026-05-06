#pragma once

namespace ttt::my_player {

// Победа - максимальный вес
constexpr int WEIGHT_WIN = 10000000;      // xxxxx - пять в ряд

// Четверки
constexpr int WEIGHT_OPEN_FOUR = 100000;   // _xxxx_ - открытая четверка
constexpr int WEIGHT_HALF_FOUR = 50000;   // _xxxx - полузакрытая четверка
constexpr int WEIGHT_GAP_FOUR = 20000;    // _x_xxx, _xx_xx, _xxx_x - четверка с брешью

// Тройки
constexpr int WEIGHT_OPEN_THREE = 30000;  // _xxx_ - открытая тройка
constexpr int WEIGHT_HALF_THREE = 15000;  // _xxx - полузакрытая тройка
constexpr int WEIGHT_GAP_THREE = 8000;    // _xx_x, _x_xx - тройка с брешью

// Двойки
constexpr int WEIGHT_OPEN_TWO = 2000;     // _xx_ - открытая двойка
constexpr int WEIGHT_HALF_TWO = 500;      // _xx - полузакрытая двойка
constexpr int WEIGHT_GAP_TWO = 300;       // двойка с брешью

// Базовые веса для случайности и позиции
constexpr int WEIGHT_POSITION = 50;      // базовый вес за позицию
constexpr int WEIGHT_CENTER = 40;        // бонус за центр
constexpr int WEIGHT_NEIGHBOR = 30;      // бонус за соседей

// Множители для атаки/защиты
constexpr int ATTACK_MULTIPLIER = 10;    // атака важнее защиты (10:1)
constexpr int DEFENSE_MULTIPLIER = 20;    // защита чуть меньше атаки

// Пороги для решений
constexpr int THREAT_THRESHOLD = 8000;   // порог для определения угрозы

} // namespace ttt::my_player