cmake_minimum_required(VERSION 3.10)

project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

include_directories(include)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PROTO_FILES
    proto/packet.proto
    proto/message.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})


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

add_executable(server ${SERVER_SRC} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(server pthread ${Protobuf_LIBRARIES})

#===== client1 =====
add_executable(client1 src/client/client1.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(client1 pthread ${Protobuf_LIBRARIES})
# ===== client2 =====
add_executable(client2 src/client/client2.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(client2 pthread ${Protobuf_LIBRARIES})