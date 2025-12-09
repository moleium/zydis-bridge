cmake_minimum_required(VERSION 3.30)
project(zydis-bridge VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET address)
    add_subdirectory(vendor/address)
endif()

if(NOT TARGET Zydis::Zydis)
    add_subdirectory(vendor/zydis)
endif()

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES FILES
        src/zydis.cppm
        src/decoder.cppm
        src/assembler.cppm
        src/types.cppm
)

target_link_libraries(${PROJECT_NAME} PRIVATE Zydis::Zydis address)
target_link_libraries(${PROJECT_NAME} PUBLIC address)
