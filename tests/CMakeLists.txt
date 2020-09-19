cmake_minimum_required(VERSION 3.7)

set(MAIN_PROJECT "asp_db")
project(${MAIN_PROJECT}-fulltests)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PROJECT_FULLTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_EXAMPLES_DIR ${PROJECT_ROOT}/examples)
set(LIBRARY_EXAMPLE_DIR ${PROJECT_EXAMPLES_DIR}/library)
set(FULLTEST_LIBRARIES "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# todo: redo this
find_package(GTest)
if(${GTEST_FOUND})
  include_directories(${GTEST_INCLUDE_DIRS})
  list(APPEND FULLTEST_LIBRARIES gtest gtest_main)

  # code coverage
  add_compile_options(-fprofile-arcs -ftest-coverage)
  list(APPEND FULLTEST_LIBRARIES gcov)

  find_package(Threads REQUIRED)
  list(APPEND FULLTEST_LIBRARIES Threads::Threads)

  include_directories(
    ${LIBRARY_EXAMPLE_DIR}
  )

  include(${PROJECT_ROOT}/db_source.cmake)

  set(MODULES_DIR ${ASP_DB_ROOT}/lib)
  set(ASP_DB_DIR ${MODULES_DIR}/asp_db)
  link_directories(${ASP_DB_DIR})

  # database tests
  add_executable(
    test_database

    ${DATABASE_SOURCE}
    ${PROJECT_FULLTEST_DIR}/test_tables.cpp
    ${PROJECT_FULLTEST_DIR}/test_queries.cpp
    ${LIBRARY_EXAMPLE_DIR}/library_tables.cpp
  )
  target_link_libraries(test_database

    asp_utils
    spdlog

    ${FULLTEST_LIBRARIES}
    pqxx
    pq
  )
endif(${GTEST_FOUND})
