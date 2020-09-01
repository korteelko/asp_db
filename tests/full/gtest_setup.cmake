find_package(GTest)

if(NOT ${GTEST_FOUND})
  set(GTEST_ROOT /opt/gtest/build)
  set(GTEST_INCLUDE_DIR /opt/gtest/googletest/include)
  message(STATUS "Try to search gtest in directory: ${GTEST_ROOT}."
    "Specify another path in ${CMAKE_CURRENT_LIST_FILE}")
  find_package(GTest)
endif()

if(NOT ${GTEST_FOUND})
  message(WARNING "gtest libraries wasn't found!")
else()
  message(STATUS "gtest was found")
endif()
