cmake_minimum_required (VERSION 3.8)

project (bayan)

#set(THREADS_PREFER_PTHREAD_FLAG ON)
set(Boost_USE_STATIC_LIBS ON)
#if (WIN32)
    set(BOOST_ROOT "C:/library/boost_1_78_0")
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "C:/library/boost_1_78_0")
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/library/boost_1_78_0/stage/lib")
#endif(WIN32)
#set(Boost_DEBUG ON)

find_package(Boost 1.78.0 REQUIRED COMPONENTS
        system
        program_options
        regex
        filesystem
        )
find_package(Threads REQUIRED)

add_executable(bayan main.cpp)

target_include_directories(bayan PRIVATE ${Boost_INCLUDE_DIR})

target_link_libraries(bayan LINK_PUBLIC "C:/library/boost_1_78_0/stage/lib" ${Boost_LIBRARIES} Threads::Threads)