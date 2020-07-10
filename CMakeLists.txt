cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME asp_db)
project(${PROJECT_NAME})


add_compile_options(-Wall)

add_definitions(-DBYCMAKE_DEBUG)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DBYCMAKE_CXX17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(MODULES_DIR ${PROJECT_ROOT}/lib)
set(OUTPUT_DIR ${MODULES_DIR}/asp_db)

if(UNIX)
  add_definitions(-DOS_NIX)
endif(UNIX)

#======================================================
# SOURCE
#======================================================
set(DATABASE_SOURCE
  ${SOURCE_ROOT}/database/db_connection.cpp
  ${SOURCE_ROOT}/database/db_connection_postgre.cpp
  ${SOURCE_ROOT}/database/db_connection_manager.cpp
  ${SOURCE_ROOT}/database/db_defines.cpp
  ${SOURCE_ROOT}/database/db_queries_setup.cpp
  ${SOURCE_ROOT}/database/db_query.cpp
)
set(UTILS_SOURCE
  ${SOURCE_ROOT}/utils/Common.cpp
  ${SOURCE_ROOT}/utils/ErrorWrap.cpp
  ${SOURCE_ROOT}/utils/Logging.cpp
)
include_directories(
  ${SOURCE_ROOT}/database
  ${SOURCE_ROOT}/utils
)

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

# todo: add examples and tests
