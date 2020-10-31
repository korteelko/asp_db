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
  // text field
  auto between_t = adb.Between<table_book>(BOOK_TITLE, "a", "z");
  std::string btstr = std::string(BOOK_TITLE_NAME) + " between 'a' and 'z'";
  EXPECT_STRCASEEQ(between_t->GetString().c_str(), btstr.c_str());

  // int field
  auto between_p = adb.Between<table_book>(BOOK_PUB_YEAR, 1920, 1985);
  std::string bpstr =
      std::string(BOOK_PUB_YEAR_NAME) + " between 1920 and 1985";
  EXPECT_STRCASEEQ(between_p->GetString().c_str(), bpstr.c_str());
  // вынесены в test_excpression.cpp
}

TEST(db_where_tree, DBTable2Operations) {
  auto eq_t = adb.Eq<table_author>(AUTHOR_NAME, "Leo Tolstoy");
  std::string eqtstr = std::string(AUTHOR_NAME_NAME) + " = 'Leo Tolstoy'";
  EXPECT_STRCASEEQ(eq_t->GetString().c_str(), eqtstr.c_str());
}
