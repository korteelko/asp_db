# .cmake файл для включения модуля как набора сырцов, а не библиотеки
set(DATABASE_SOURCE
  "${ASP_DB_ROOT}/source/database/db_connection.cpp"
  "${ASP_DB_ROOT}/source/database/db_connection_postgre.cpp"
  "${ASP_DB_ROOT}/source/database/db_connection_manager.cpp"
  "${ASP_DB_ROOT}/source/database/db_defines.cpp"
  "${ASP_DB_ROOT}/source/database/db_queries_setup.cpp"
  "${ASP_DB_ROOT}/source/database/db_query.cpp"
)

include_directories(${ASP_DB_ROOT}/source/database)
