cmake_minimum_required(VERSION 3.12)
project(EternalResourceExtractor)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES
        ./main.cpp
        ./utils.cpp
        ./utils.hpp
        ./ooz.hpp
        ./mmap/mmap.cpp
        ./mmap/mmap.hpp
        ./argh/argh.hpp
        )

if (MSVC)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_CXX_FLAGS "/O2 /Oi /Ot /GL /EHsc")
else()
        set (CMAKE_CXX_FLAGS "-Ofast -DNDEBUG -Wno-unused-result")
endif()

add_executable(EternalResourceExtractor ${SOURCES})

if(MSVC)
        target_link_options(EternalResourceExtractor PUBLIC "/LTCG")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        target_link_libraries(EternalResourceExtractor ${CMAKE_SOURCE_DIR}/lib/ooz.lib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(EternalResourceExtractor ${CMAKE_DL_LIBS} ${CMAKE_SOURCE_DIR}/lib/libooz.a)
endif()
