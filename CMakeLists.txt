cmake_minimum_required(VERSION 3.2)
project(update_lite)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic")

set(SOURCE_FILES main.cpp Simulator.cpp Simulator.h Settings.cpp Settings.h TermPack.cpp TermPack.h Consolidation.cpp Consolidation.h Report.cpp Report.h Caching.cpp Caching.h Landlord.h Landlord.cpp)
add_executable(update_lite ${SOURCE_FILES})

target_link_libraries( update_lite pthread)
