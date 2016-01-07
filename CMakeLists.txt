cmake_minimum_required (VERSION 2.8.8)
project (tlg2png)
set (CMAKE_CXX_STANDARD 14)

# ------------
# Dependencies
# ------------
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})
link_directories(${ZLIB_LIBRARY_DIRS})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})
link_directories(${PNG_LIBRARY_DIRS})

# --------------------
# Global build options
# --------------------
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # for MinGW-w64
endif()

# ------------
# Source files
# ------------
set(CMAKE_SOURCE_DIR "${CMAKE_BINARY_DIR}/../")
file(GLOB_RECURSE sources "${CMAKE_SOURCE_DIR}/src/*.cc")
file(GLOB_RECURSE headers "${CMAKE_SOURCE_DIR}/src/*.h")

# -------------------
# Linking definitions
# -------------------
add_executable(tlg2png ${sources} ${headers})
target_link_libraries(tlg2png ${ZLIB_LIBRARIES} ${PNG_LIBRARIES})
