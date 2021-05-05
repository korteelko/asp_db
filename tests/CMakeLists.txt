set(TARGET_ASP_DB_TESTS asp_db-fulltests)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(PROJECT_FULLTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARY_EXAMPLE_DIR ${PROJECT_ROOT}/examples/library)

find_package(GTest)
if(${GTEST_FOUND})
  add_executable(
    ${TARGET_ASP_DB_TESTS}

    ${PROJECT_ROOT}/source/db_connection.cpp
    ${PROJECT_ROOT}/source/db_connection_manager.cpp
    ${PROJECT_ROOT}/source/db_defines.cpp
    ${PROJECT_ROOT}/source/db_expression.cpp
    ${PROJECT_ROOT}/source/db_queries_setup.cpp
    ${PROJECT_ROOT}/source/db_queries_setup_select.cpp
    ${PROJECT_ROOT}/source/db_query.cpp
    ${PROJECT_FULLTEST_DIR}/test_connection.cpp
    ${PROJECT_FULLTEST_DIR}/test_expression.cpp
    ${PROJECT_FULLTEST_DIR}/test_tables.cpp
    ${PROJECT_FULLTEST_DIR}/test_queries.cpp
    ${LIBRARY_EXAMPLE_DIR}/library_tables.cpp
    ${OPTIONAL_SRC}
  )
  target_compile_options(${TARGET_ASP_DB_TESTS}
    PRIVATE -fprofile-arcs -ftest-coverage -Wall)

  target_include_directories(${TARGET_ASP_DB_TESTS}
    PRIVATE ${LIBRARY_EXAMPLE_DIR}
    PRIVATE ${PROJECT_ROOT}/include
    PRIVATE ${PROJECT_ROOT}/lib/asp_utils/include
    PRIVATE ${GTEST_INCLUDE_DIRS}
  )

  target_link_libraries(${TARGET_ASP_DB_TESTS}
    asp_utils
    ${OPTIONAL_LIBS}
    gtest gtest_main
    gcov
  )
endif(${GTEST_FOUND})
