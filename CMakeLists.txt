cmake_minimum_required(VERSION 3.27)
project(edupage C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

enable_testing()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
   add_subdirectory(tests)
endif()