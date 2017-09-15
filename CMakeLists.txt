# min version req'd
cmake_minimum_required(VERSION 3.1)

# name of project
project(meshing)

# add C++11 standard
set(CMAKE_CXX_STANDARD 11)

# set to release build type
set(CMAKE_BUILD_TYPE Release)

# output folder for binaries
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# compile flags and library locations
if(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS "/EHsc /W3 /MT /std:c++latest")
    include_directories(${CMAKE_SOURCE_DIR}/include)
    link_directories(${CMAKE_SOURCE_DIR}/lib "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/x64")
    set(PROJECT_LINK_LIBS glfw3dll glew32 OpenGL32 )
    file(GLOB DLL_FILES "lib/*.dll")
    file(COPY ${DLL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Release NO_SOURCE_PERMISSIONS)
    file(COPY ${DLL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/Debug NO_SOURCE_PERMISSIONS)
elseif(UNIX)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -Wfatal-errors -pthread")
    include_directories("/usr/include")
    link_directories("/usr/lib")
    set(PROJECT_LINK_LIBS GLEW GL glfw)
endif()

# wildcard add source files like so:
file(GLOB SOURCES "src/*.cpp")

add_executable(main ${SOURCES})
target_link_libraries(main ${PROJECT_LINK_LIBS})
