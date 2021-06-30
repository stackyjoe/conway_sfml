cmake_minimum_required(VERSION 3.5)

project(game_of_life LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(game_of_life main.cpp)
link_directories("/usr/lib/")
link_directories("/usr/local/lib/")

target_link_libraries(game_of_life c++ /usr/local/lib/libtbb.so)



#set(TBB_LIBRARY "libtbb.so")

#set(TBB_LIBRARY_MALLOC "libtbbmalloc.so")

find_package(SFML REQUIRED COMPONENTS system window graphics audio)
target_link_libraries(game_of_life sfml-system sfml-window sfml-graphics sfml-audio)
include_directories("/usr/local/include/")

find_package(Lua REQUIRED)
target_link_libraries(game_of_life ${LUA_LIBRARIES})
include_directories("/usr/local/include/lua53")


find_library(AVCODEC_LIBRARY avcodec)
target_link_libraries( game_of_life ${AVCODEC_LIBRARY})


find_library(AVFORMAT_LIBRARY avformat)
target_link_libraries( game_of_life ${AVFORMAT_LIBRARY})


find_library(AVUTIL_LIBRARY avutil)
target_link_libraries( game_of_life ${AVUTIL_LIBRARY})

find_library(SWSCALE_LIBRARY swscale)
target_link_libraries(game_of_life ${SWSCALE_LIBRARY})

find_library(X264_LIBRARY x264)
target_link_libraries(game_of_life ${X264_LIBRARY})
