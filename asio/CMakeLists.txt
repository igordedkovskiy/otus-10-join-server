cmake_minimum_required(VERSION 3.5)

project(asio)

find_package(Boost COMPONENTS system REQUIRED)
find_package(Threads)
include_directories(${Boost_INCLUDE_DIR})

add_executable(asio_async_server asio_async_server.cpp)

set_target_properties(asio_async_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(asio_async_server PRIVATE
    wsock32
    ws2_32
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(asio_async_server2 asio_async_server2.cpp)

set_target_properties(asio_async_server2 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_compile_options(asio_async_server2 PRIVATE
    "-fcoroutines"
)

target_link_libraries(asio_async_server2 PRIVATE
    wsock32
    ws2_32
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

