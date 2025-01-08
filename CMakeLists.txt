cmake_minimum_required(VERSION 3.27)
project(openbw_desync_finder)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)

set(OPENBW_DIR ${CMAKE_SOURCE_DIR}/3rdparty/openbw/openbw)
add_subdirectory(3rdparty/openbw/bwapi)

include_directories(SYSTEM
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/include
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/Util/Source
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/BWAPI/openbw
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/BWAPI/openbw/BWAPI
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/BWAPI/Source
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/BWAPICore
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/OpenBWData
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/bwapi/bwapi/Shared
        ${CMAKE_SOURCE_DIR}/3rdparty/openbw/openbw
)

add_executable(dump_data src/dump_data.cpp)
add_executable(check_for_desync src/check_for_desync.cpp)
add_executable(standalone_checker src/standalone_checker.cpp)

target_link_libraries(dump_data BWAPI)
target_link_libraries(check_for_desync BWAPI)
target_link_libraries(standalone_checker BWAPI)
