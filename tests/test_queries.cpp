#include "db_queries_setup.h"
#include "db_queries_setup_select.h"
#include "db_tables.h"
#include "library_tables.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>
#include <numeric>

#include <assert.h>

LibraryDBTables adb;

TEST(db_where_tree, DBTableBetween) {
  // todo: не прозрачна связь между параметром шаблона и id столбца
  auto d =
      adb.Between<table_book>(BOOK_TITLE, std::string("a"), std::string("z"));
  // вынесены в test_excpression.cpp
}
