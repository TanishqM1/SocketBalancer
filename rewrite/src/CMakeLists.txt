# config
cmake_minimum_required(VERSION 3.16)
project(SocketBalancer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# WINDOWS ONLY
if (WIN32)
    set(EXTRA_LIBS ws2_32)
endif()

add_executable(im_server im_server.cpp)
target_link_libraries(im_server PRIVATE ${EXTRA_LIBS})

add_executable(im_client im_client.cpp)
target_link_libraries(im_client PRIVATE ${EXTRA_LIBS})

add_executable(load_balancer load_balancer.cpp)
target_link_libraries(load_balancer PRIVATE ${EXTRA_LIBS})
