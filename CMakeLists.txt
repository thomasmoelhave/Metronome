cmake_minimum_required(VERSION 2.6)

project(metronome)
set(TS_BOOST_COMPONENTS system filesystem program_options)
find_package(Boost COMPONENTS ${TS_BOOST_COMPONENTS} REQUIRED)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)

add_executable(metronome metronome.cpp)
target_link_libraries(metronome ${Boost_LIBRARIES})

configure_file("metronome-list.sh" ${CMAKE_BINARY_DIR}/metronome-list.sh)

find_package(Boost)
