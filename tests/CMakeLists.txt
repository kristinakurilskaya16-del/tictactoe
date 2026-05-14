cmake_minimum_required(VERSION 3.20.0)

add_executable(ci_tests ci_tests.cpp)
target_link_libraries(ci_tests tttplayer gtest gtest_main pthread)

target_include_directories(ci_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..      
    ${CMAKE_CURRENT_SOURCE_DIR}/../src 
)

add_test(NAME ci_tests COMMAND ci_tests)

# Always built targets (no dependency on BUILD_TTTCORE)
add_executable(test_my_player test_my_player.cpp)
target_link_libraries(test_my_player tttplayer)
add_test(NAME smoke_test_player COMMAND ./test_my_player)

add_executable(test_stats test_stats.cpp)
target_link_libraries(test_stats tttplayer)
add_test(NAME test_player_stats COMMAND ./test_stats)

# Targets that require full or prebuilt tttcore
if((BUILD_TTTCORE STREQUAL "FULL") OR (BUILD_TTTCORE STREQUAL "PREBUILT"))
  # Baseline tests
  add_executable(test_with_baseline test_baseline.cpp)
  target_link_libraries(test_with_baseline tttplayer)
  add_test(NAME test_againts_baseline COMMAND ./test_with_baseline)

  # New stat test
  add_executable(test_stats_vs_baseline test_stats_vs_baseline.cpp)
  target_link_libraries(test_stats_vs_baseline tttplayer)
  add_test(NAME test_stats_vs_baseline COMMAND ./test_stats_vs_baseline)
  
  # Human player test
  add_executable(test_my_player_vs_human test_my_player_vs_human.cpp human_player.cpp)
  target_link_libraries(test_my_player_vs_human tttplayer)
endif()
