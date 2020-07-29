cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME asp_db_meta_example)
project(${PROJECT_NAME})


add_compile_options(-Wall)

set(CMAKE_BUILD_TYPE Debug)
add_definitions(-DBYCMAKE_DEBUG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DBYCMAKE_CXX17)

set(ASP_DB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(MODULES_DIR ${ASP_DB_ROOT}/lib)
set(ASP_DB_DIR ${MODULES_DIR}/asp_db)

message(STATUS "link directory: ${ASP_DB_DIR}")
link_directories(${ASP_DB_DIR})

if(UNIX)
  add_definitions(-DOS_NIX)
endif(UNIX)

#======================================================
# SOURCE
#======================================================
include_directories(
  ${ASP_DB_ROOT}/source/database
  ${ASP_DB_ROOT}/source/utils
)

#======================================================
# LIBRARIES AND MODULES
#======================================================
#   pqxx
#   http://pqxx.org/development/libpqxx/
add_definitions(-DBYCMAKE_WITH_POSTGRESQL)
set(PQXX_LIBS pqxx pq)

#   thread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}

  asp_db
  ${PQXX_LIBS}
)
