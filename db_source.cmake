# .cmake файл для включения модуля как набора сырцов, а не библиотеки
set(DATABASE_SOURCE
  database/db_connection.cpp
  database/db_connection_postgre.cpp
  database/db_connection_manager.cpp
  database/db_defines.cpp
  database/db_queries_setup.cpp
  database/db_query.cpp
)
include_directories(database)
