message(STATUS "\t\tRun Database tests")

add_definitions(-DDATABASE_TEST)
set (COMMON_SRC
  ${DB_SOURCE_DIR}/db_connection.cpp
  ${DB_SOURCE_DIR}/db_connection_manager.cpp
  ${DB_SOURCE_DIR}/db_connection_postgre.cpp
  ${DB_SOURCE_DIR}/db_defines.cpp
  ${DB_SOURCE_DIR}/db_queries_setup.cpp
  ${DB_SOURCE_DIR}/db_query.cpp

  ${UTILS_SOURCE_DIR}/Common.cpp
  ${UTILS_SOURCE_DIR}/ErrorWrap.cpp
  ${UTILS_SOURCE_DIR}/Logging.cpp

  ${LIBRARY_EXAMPLE_DIR}/library_tables.cpp
)
# tests database
add_executable(
  test_database

  ${ASP_DB_FULLTEST_DIR}/database/test_tables.cpp
  ${ASP_DB_FULLTEST_DIR}/database/test_queries.cpp
  ${COMMON_SRC}
)

target_link_libraries(test_database

  ${GTEST_LIBRARIES}
  ${FULLTEST_LIBRARIES}
  pqxx
  pq
)

add_test(test_database "database")
