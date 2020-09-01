cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME asp_db)
project(${PROJECT_NAME})


add_compile_options(-Wall)

set(CMAKE_BUILD_TYPE Debug)
add_definitions(-DBYCMAKE_DEBUG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DBYCMAKE_CXX17)

set(ASP_DB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(MODULES_DIR ${ASP_DB_ROOT}/lib)
set(OUTPUT_DIR ${MODULES_DIR}/asp_db)

if(UNIX)
  add_definitions(-DOS_NIX)
endif(UNIX)

# OPTIONS
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  option(BUILD_EXAMPLES "Build examples" ON)
  option(TESTS_ENABLED "Run tests" ON)
endif()

#======================================================
# SOURCE
#======================================================
include(db_source.cmake)
include(utils_source.cmake)

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

#======================================================
# OUTPUT SETUP
#======================================================
add_library(${PROJECT_NAME} STATIC ${DATABASE_SOURCE} ${UTILS_SOURCE})
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set_target_properties(${PROJECT_NAME}
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR}
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR}
  )
endforeach()

target_link_libraries(${PROJECT_NAME}

  stdc++fs
  ${PQXX_LIBS}
  Threads::Threads
)

# add examples
if(BUILD_EXAMPLES)
  message(STATUS "Собираем примеры")
  add_subdirectory("${ASP_DB_ROOT}/examples")
endif()

# run tests
if(TESTS_ENABLED)
  message(STATUS "Запускаем тесты")
  add_subdirectory("${ASP_DB_ROOT}/tests/full")
endif()
