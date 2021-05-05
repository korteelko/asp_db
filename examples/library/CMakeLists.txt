set(PROJECT_NAME asp_db_library_example)
set(TARGET_EXAMPLE ${PROJECT_NAME})

set(ASP_DB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# set target
add_executable(${TARGET_EXAMPLE} library_tables.cpp main.cpp)

target_compile_features(${TARGET_EXAMPLE} PUBLIC cxx_std_20)
target_include_directories(${TARGET_EXAMPLE} PRIVATE ${ASP_DB_INCLUDE})

target_link_libraries(${TARGET_EXAMPLE}
  asp_db
)
