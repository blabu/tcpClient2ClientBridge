cmake_minimum_required(VERSION 3.0)
set(PROJ_NAME com2tcp)
project(${PROJ_NAME})
set(CMAKE_BUILD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PROJ_NAME}/build)

FILE(GLOB MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
                   ${CMAKE_CURRENT_SOURCE_DIR}/clients/*.cpp
                   ${CMAKE_CURRENT_SOURCE_DIR}/Logger/*.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ 
                    ${CMAKE_CURRENT_SOURCE_DIR}/clients/ 
                    ${CMAKE_CURRENT_SOURCE_DIR}/Logger/ )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -fPIC -static")
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(${PROJ_NAME} ${MAIN_SRC})

message(STATUS "Try find boost library with static linked")

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
find_package(Boost REQUIRED COMPONENTS system thread regex chrono)
if(NOT Boost_FOUND) 
    message(FATAL_ERROR "Can not find boost system, thread, chrono and regex libraries")
    return()
else()
    message(STATUS "Find Boost version ${Boost_VERSION} at ${Boost_LIBRARY_DIRS}")
endif()

target_include_directories(${PROJ_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJ_NAME} ${Boost_LIBRARIES})