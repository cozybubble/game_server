cmake_minimum_required(VERSION 3.10)

project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# ===== server =====
file(GLOB SERVER_SRC
    src/server.cpp
    src/connection.cpp
    src/player.cpp
    src/player_manager.cpp
    src/main.cpp
    src/match_manager.cpp
    src/room.cpp
    src/room_manager.cpp
)

add_executable(server ${SERVER_SRC})
target_link_libraries(server pthread)

# ===== client1 =====
add_executable(client1 src/client/client1.cpp)
target_link_libraries(client1 pthread)

# ===== client2 =====
add_executable(client2 src/client/client2.cpp)
target_link_libraries(client2 pthread)