cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME asp_db)
project(${PROJECT_NAME})
set(TARGET_ASP_DB_LIB ${PROJECT_NAME})

set(ASP_DB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(MODULES_DIR ${ASP_DB_ROOT}/lib)
set(OUTPUT_DIR ${ASP_DB_ROOT}/out)

message(STATUS "${PROJECT_NAME} build type: ${CMAKE_BUILD_TYPE}")

option(WITH_POSTGRESQL "Build with postres libs: `pq` and `pqxx`" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(TESTS_ENABLED "Run tests" OFF)

if(WITH_POSTGRESQL)
  message(STATUS "Add libraries pq and pqxx.\n\t\t"
    "See http://pqxx.org/development/libpqxx for more information")
  list(APPEND OPTIONAL_SRC ${ASP_DB_ROOT}/source/db_connection_postgre.cpp)
  list(APPEND OPTIONAL_LIBS pq pqxx)
  # прокинуть дефайн на postgres
  # add_definitions(-DBYCMAKE_WITH_POSTGRESQL)
endif()

add_library(
  ${TARGET_ASP_DB_LIB}
  ${ASP_DB_ROOT}/source/db_connection.cpp
  ${ASP_DB_ROOT}/source/db_connection_manager.cpp
  ${ASP_DB_ROOT}/source/db_defines.cpp
  ${ASP_DB_ROOT}/source/db_expression.cpp
  ${ASP_DB_ROOT}/source/db_queries_setup.cpp
  ${ASP_DB_ROOT}/source/db_queries_setup_select.cpp
  ${ASP_DB_ROOT}/source/db_query.cpp
  ${OPTIONAL_SRC})

target_compile_features(${TARGET_ASP_DB_LIB} PUBLIC cxx_std_17)
target_include_directories(${TARGET_ASP_DB_LIB} PUBLIC ${SOURCE_ROOT})
if(WITH_POSTGRESQL)
  target_compile_definitions(${TARGET_ASP_DB_LIB}
                             PUBLIC -DBYCMAKE_WITH_POSTGRESQL)
endif()

add_subdirectory(${MODULES_DIR}/asp_utils)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(${TARGET_ASP_DB_LIB} PRIVATE -DBYCMAKE_DEBUG)
endif()

find_package(Threads REQUIRED)
set_target_properties(
  ${TARGET_ASP_DB_LIB} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
                                  ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

target_link_libraries(${TARGET_ASP_DB_LIB} Threads::Threads stdc++fs asp_utils
                      ${OPTIONAL_LIBS})

# add examples
if(BUILD_EXAMPLES)
  message(STATUS "Собираем примеры")
  add_subdirectory("${ASP_DB_ROOT}/examples")
endif()

# run tests
if(TESTS_ENABLED)
  message(STATUS "Запускаем тесты")
  enable_testing()
  add_subdirectory("${ASP_DB_ROOT}/tests")
endif()

# генерировать .json файл информации о компилируемых объектниках
#   compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# скопировать файл compile_commands.json в корень проекта, для работы
#   синтаксических анализаторов, cppcheck и т.д.
add_custom_target(
  copy-compile-commands ALL
  ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR})
