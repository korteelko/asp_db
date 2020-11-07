set(PROJECT_NAME asp_db_library_example)
set(TARGET_EXAMPLE ${PROJECT_NAME})

set(ASP_DB_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../source)

# set target
add_executable(${TARGET_EXAMPLE} library_tables.cpp main.cpp)

target_include_directories(${TARGET_EXAMPLE} PRIVATE ${ASP_DB_SOURCE_ROOT})

target_link_libraries(${TARGET_EXAMPLE}
  asp_utils
  asp_db
)
