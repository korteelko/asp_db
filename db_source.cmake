set(DATABASE_SOURCE
  "${ASP_DB_ROOT}/source/db_connection.cpp"
  "${ASP_DB_ROOT}/source/db_connection_postgre.cpp"
  "${ASP_DB_ROOT}/source/db_connection_manager.cpp"
  "${ASP_DB_ROOT}/source/db_defines.cpp"
  "${ASP_DB_ROOT}/source/db_expression.cpp"
  "${ASP_DB_ROOT}/source/db_queries_setup.cpp"
  "${ASP_DB_ROOT}/source/db_queries_setup_select.cpp"
  "${ASP_DB_ROOT}/source/db_query.cpp"
)

include_directories(${ASP_DB_ROOT}/source)
