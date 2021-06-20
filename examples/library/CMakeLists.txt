set(PROJECT_NAME asp_db_library_example)
set(TARGET_EXAMPLE ${PROJECT_NAME})

set(ASP_DB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_executable(${TARGET_EXAMPLE} library_tables.cpp main.cpp)
add_system_defines(${TARGET_EXAMPLE})
target_compile_definitions(${TARGET_EXAMPLE} PRIVATE
  DEBUG_POSTGRESQL
  # DEBUG_FIREBIRD
)

target_include_directories(${TARGET_EXAMPLE} PRIVATE ${ASP_DB_INCLUDE})

target_link_libraries(${TARGET_EXAMPLE}
  asp_db
)
