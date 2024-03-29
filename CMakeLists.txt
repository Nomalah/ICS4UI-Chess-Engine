cmake_minimum_required(VERSION 3.17)

project(NomalahChess)

set(CMAKE_CXX_STANDARD 20)

set(INCLUDES
    include/ai.hpp
    include/chess.hpp
    include/chess_constants.hpp
    include/chess_types.hpp
    include/chess_utils.hpp)

set(HEADERS
    src/debugging&Util.cpp
    src/moveGen.cpp
    src/moveMaking.cpp
    src/userInterface.cpp)

add_library(NomalahChessLib STATIC ${INCLUDES} ${HEADERS})
target_include_directories(NomalahChessLib PUBLIC include)

add_executable(NomalahChess engine.cpp)
target_link_libraries(NomalahChess NomalahChessLib)
add_executable(SelfPlayNomalahChess ai.cpp)
target_link_libraries(SelfPlayNomalahChess NomalahChessLib)
add_executable(UserPlayNomalahChess aiVsPlayer.cpp)
target_link_libraries(UserPlayNomalahChess NomalahChessLib)
add_executable(PerftNomalahChess perft.cpp)
target_link_libraries(PerftNomalahChess NomalahChessLib)
add_executable(LichessNomalahChess lichessEngine.cpp)
target_link_libraries(LichessNomalahChess NomalahChessLib)
