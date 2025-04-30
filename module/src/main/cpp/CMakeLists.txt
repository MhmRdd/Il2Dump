cmake_minimum_required(VERSION 3.22.1)
project(sample)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
    set(CXX_FLAGS "${CXX_FLAGS} -O0 -g -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
else ()
    set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
endif ()

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

add_library(${MODULE_NAME} SHARED il2dump.cpp)
target_link_libraries(${MODULE_NAME} cxx log)

add_subdirectory(external)
