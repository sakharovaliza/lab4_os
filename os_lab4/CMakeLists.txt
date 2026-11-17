cmake_minimum_required(VERSION 3.14)
project(os_lab4 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Receiver main.cpp)
add_executable(Sender   sender.cpp)

target_link_libraries(Receiver pthread)
target_link_libraries(Sender   pthread)