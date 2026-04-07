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
)

add_executable(server ${SERVER_SRC})
target_link_libraries(server pthread)

# ===== client =====
add_executable(client src/client.cpp)
target_link_libraries(client pthread)