cmake_minimum_required(VERSION 3.7)

set(MAIN_PROJECT "asp_db")
project(${MAIN_PROJECT}-fulltests)

include(gtest_setup.cmake)
set(ASP_DB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../)
set(ASP_DB_FULLTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ASP_DB_EXAMPLES_DIR ${ASP_DB_ROOT}/examples)
set(LIBRARY_EXAMPLE_DIR ${ASP_DB_EXAMPLES_DIR}/library)
set(FULLTEST_LIBRARIES "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# code coverage
add_compile_options(-fprofile-arcs -ftest-coverage)
list(APPEND FULLTEST_LIBRARIES gcov)

set(UTILS_SOURCE_DIR "${ASP_DB_ROOT}/source/utils")
set(DB_SOURCE_DIR "${ASP_DB_ROOT}/source/database")

if(${GTEST_FOUND})
  find_package(Threads REQUIRED)
  list(APPEND FULLTEST_LIBRARIES Threads::Threads)

  include_directories(
    ${UTILS_SOURCE_DIR}
    ${DB_SOURCE_DIR}
    ${LIBRARY_EXAMPLE_DIR}

    ${GTEST_INCLUDE_DIRS}
  )
  enable_testing()
  # database tests
  include(${ASP_DB_FULLTEST_DIR}/database/database_tests.cmake)
endif(${GTEST_FOUND})
