cmake_minimum_required(VERSION 3.5)

project(asio)

#add_definitions(-D__USE_W32_SOCKETS)# -D_WIN32_WINNT=0x0501 -DWINVER=0x0501)

find_package(Boost COMPONENTS system REQUIRED)
find_package(Threads)
include_directories(${Boost_INCLUDE_DIR})

#add_executable(asio_server asio_server.cpp)

#set_target_properties(asio_server PROPERTIES
#        CXX_STANDARD 14
#        CXX_STANDARD_REQUIRED ON
#        )

#target_link_libraries(asio_server
#        ${Boost_LIBRARIES}
#        ${CMAKE_THREAD_LIBS_INIT}
#        )


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



