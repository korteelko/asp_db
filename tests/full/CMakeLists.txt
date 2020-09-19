cmake_minimum_required(VERSION 3.7)

set(MAIN_PROJECT "asp_db")
project(${MAIN_PROJECT}-fulltests)

set(ASP_DB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(ASP_DB_FULLTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ASP_DB_EXAMPLES_DIR ${ASP_DB_ROOT}/examples)
set(LIBRARY_EXAMPLE_DIR ${ASP_DB_EXAMPLES_DIR}/library)
set(FULLTEST_LIBRARIES "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${ASP_DB_ROOT}/lib/googletest" "lib/googletest")
list(APPEND FULLTEST_LIBRARIES gtest gtest_main)

# code coverage
add_compile_options(-fprofile-arcs -ftest-coverage)
list(APPEND FULLTEST_LIBRARIES gcov)

find_package(Threads REQUIRED)
list(APPEND FULLTEST_LIBRARIES Threads::Threads)

include_directories(
  ${LIBRARY_EXAMPLE_DIR}
)

include(${ASP_DB_ROOT}/db_source.cmake)
include(${ASP_DB_ROOT}/utils_source.cmake)
# include(GoogleTest)

# database tests
add_executable(
  test_database

  ${UTILS_SOURCE}
  ${DATABASE_SOURCE}
  ${ASP_DB_FULLTEST_DIR}/database/test_tables.cpp
  ${ASP_DB_FULLTEST_DIR}/database/test_queries.cpp
  ${LIBRARY_EXAMPLE_DIR}/library_tables.cpp
)
target_link_libraries(test_database

  ${FULLTEST_LIBRARIES}
  pqxx
  pq
)

# utils tests
add_executable(
  test_utils

  ${UTILS_SOURCE}
  ${ASP_DB_FULLTEST_DIR}/utils/test_logging.cpp
)
target_link_libraries(test_utils ${FULLTEST_LIBRARIES})
