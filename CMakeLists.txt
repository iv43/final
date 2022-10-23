cmake_minimum_required(VERSION 3.22)
project(final)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99i")

set(LIBS "-levent")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -levent")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -levent")

add_executable(${PROJECT_NAME} libeventserver.cpp)
target_link_libraries(${PROJECT_NAME} ${LIBS})
