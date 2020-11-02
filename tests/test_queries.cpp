#include "db_queries_setup.h"
#include "db_queries_setup_select.h"
#include "db_tables.h"
#include "db_where.h"
#include "library_tables.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>
#include <numeric>

#include <assert.h>

LibraryDBTables ldb;

TEST(db_where_tree, DBTableBetween) {
  // text field
  WhereTreeConstructor<table_book> adb(&ldb);
  // todo: не прозрачна связь между параметром шаблона и id столбца
  auto between_t = adb.Between(BOOK_TITLE, "a", "z");
  std::string btstr = std::string(BOOK_TITLE_NAME) + " between 'a' and 'z'";
  EXPECT_STRCASEEQ(between_t->GetString().c_str(), btstr.c_str());

  // int field
  auto between_p = adb.Between(BOOK_PUB_YEAR, 1920, 1985);
  std::string bpstr =
      std::string(BOOK_PUB_YEAR_NAME) + " between 1920 and 1985";
  EXPECT_STRCASEEQ(between_p->GetString().c_str(), bpstr.c_str());
  // вынесены в test_excpression.cpp
}

TEST(db_where_tree, DBTable2Operations) {
  WhereTreeConstructor<table_author> adb(&ldb);
  auto eq_t = adb.Eq(AUTHOR_NAME, "Leo Tolstoy");
  std::string eqtstr = std::string(AUTHOR_NAME_NAME) + " = 'Leo Tolstoy'";
  EXPECT_STRCASEEQ(eq_t->GetString().c_str(), eqtstr.c_str());
  // todo: add cases for other operators
}

TEST(db_where_tree, DBTableAnd) {
  WhereTreeConstructor<table_book> adb(&ldb);
  auto eq_t1 = adb.Eq(BOOK_TITLE, "Hobbit");
  // todo: how about instatnces for enums???
  auto eq_t2 = adb.Eq(BOOK_LANG, (int)lang_eng);
  auto gt_t3 = adb.Gt(BOOK_PUB_YEAR, 1900);

  // check
  //   and1
  auto and1 = adb.And(eq_t1, eq_t2);
  std::string and1_s = "(" + std::string(BOOK_TITLE_NAME) + " = 'Hobbit'" +
                       ") AND (" + BOOK_LANG_NAME + " = 2)";
  EXPECT_STRCASEEQ(and1->GetString().c_str(), and1_s.c_str());

  //   and2
  auto and2 = adb.And(eq_t1, gt_t3);
  std::string and2_s = "(" + std::string(BOOK_TITLE_NAME) + " = 'Hobbit'" +
                       ") AND (" + BOOK_PUB_YEAR_NAME + " > 1900)";
  EXPECT_STRCASEEQ(and2->GetString().c_str(), and2_s.c_str());

  //   and3
  auto and3 = adb.And(eq_t1, eq_t2, gt_t3);
  std::string and3_s = "((" + std::string(BOOK_TITLE_NAME) + " = 'Hobbit'" +
                       ") AND (" + BOOK_LANG_NAME + " = 2" +
                       ")) AND (" BOOK_PUB_YEAR_NAME + " > 1900)";
  EXPECT_STRCASEEQ(and3->GetString().c_str(), and3_s.c_str());

  // check nullptr
  // and4
  wns::node_ptr null_t4 = nullptr;
  eq_t1 = adb.Eq(BOOK_TITLE, "Hobbit");
  auto and4 = adb.And(eq_t1, null_t4);
  std::string and4_s = std::string(BOOK_TITLE_NAME) + " = 'Hobbit'";
  EXPECT_STRCASEEQ(and4->GetString().c_str(), and4_s.c_str());

  // and5
  wns::node_ptr null_t5 = nullptr;
  auto and5 = adb.And(null_t4, null_t5);
  EXPECT_STRCASEEQ(and5->GetString().c_str(), "");
}

TEST(WhereTreeConstructor, Init) {
  WhereTreeConstructor<table_translation> ts(&ldb);
  WhereTree<table_translation> wt(ts);
  wt.Init(ts.And(ts.Ge(TRANS_ID, 3), ts.Lt(TRANS_ID, 10),
                 ts.Eq(TRANS_LANG, int(lang_rus))));
  // проверим результат инициализации
  std::string wts = std::string("((") + TRANS_ID_NAME + " >= 3) AND (" +
                    TRANS_ID_NAME + " < 10)) AND (" + TRANS_LANG_NAME + " = " +
                    std::to_string(int(lang_rus)) + ")";
  EXPECT_STRCASEEQ(wt.GetWhereTree()->GetString().c_str(), wts.c_str());
  // продолжим
  wt.AddOr(ts.And(ts.Gt(TRANS_ID, 13), ts.Lt(TRANS_ID, 15)));
  // снова проверим
  std::string wfs = std::string("(((") + TRANS_ID_NAME + " >= 3) AND (" +
                    TRANS_ID_NAME + " < 10)) AND (" + TRANS_LANG_NAME + " = " +
                    std::to_string(int(lang_rus)) + ")) OR ((" + TRANS_ID_NAME +
                    " > 13" + ") AND (" + TRANS_ID_NAME + " < 15))";
  EXPECT_STRCASEEQ(wt.GetWhereTree()->GetString().c_str(), wfs.c_str());
}
