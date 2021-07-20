cmake_minimum_required(VERSION 3.12)
project(EternalResourceExtractor)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        ./main.c
        ./utils.c
        ./utils.h
        ./linoodle_lib.h
        ./oo2core_dll.h
        ./getdelim.c
        )

set (CMAKE_C_FLAGS "-O2 -DNDEBUG -Wno-unused-result")
add_executable(EternalResourceExtractor ${SOURCES})

target_link_libraries(EternalResourceExtractor ${CMAKE_DL_LIBS})
