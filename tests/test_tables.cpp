#include "db_connection.h"
#include "db_connection_manager.h"
#include "library_structs.h"
#include "library_tables.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>

#include <assert.h>


using namespace asp_db;
LibraryDBTables adb_;

/** \brief Тестинг работы с БД */
class DatabaseTablesTest: public ::testing::Test {
protected:
  DatabaseTablesTest()
    : dbm_(&adb_) {
    db_parameters db_par;
    db_par.is_dry_run = false;
    db_par.supplier = db_client::POSTGRESQL;
    db_par.name = "labibliotecadebabel";
    db_par.username = "jorge";
    db_par.password = "my_pass";
    db_par.host = "127.0.0.1";
    db_par.port = 5432;
    EXPECT_TRUE(is_status_aval(dbm_.ResetConnectionParameters(db_par)));
    if (dbm_.GetErrorCode()) {
      std::cerr << dbm_.GetErrorCode();
      EXPECT_TRUE(false);
    }
  }
  ~DatabaseTablesTest() {}

protected:
  /** \brief Менеджер подключения к БД */
  DBConnectionManager dbm_;
};

TEST_F(DatabaseTablesTest, TableExists) {
  ASSERT_TRUE(dbm_.GetErrorCode() == ERROR_SUCCESS_T);
  std::vector<db_table> tables { table_book,
      table_translation, table_author };
  for (const auto &x: tables) {
    if (!dbm_.IsTableExists(x)) {
      dbm_.CreateTable(x);
      ASSERT_TRUE(dbm_.IsTableExists(x));
    }
  }
}

/** \brief Тест на добавлени строки к таблице моделей */
TEST_F(DatabaseTablesTest, InsertModelInfo) {
  std::string str = "string by gtest";
  /* insert */
  book divine_comedy;
  divine_comedy.id = -1;
  divine_comedy.lang = lang_ita;
  divine_comedy.title = "Divina Commedia";
  // дата завершения
  divine_comedy.first_pub_year = 1320;
  divine_comedy.initialized = book::f_full & ~book::f_id;
  dbm_.SaveSingleRow(divine_comedy, nullptr);

  /* select */
  std::vector<book> r;
  auto st = dbm_.SelectRows(divine_comedy, &r);

  ASSERT_TRUE(r.size() > 0);
  EXPECT_GT(r[0].id, 0);
  EXPECT_EQ(r[0].lang, lang_ita);
  EXPECT_EQ(r[0].title, "Divina Commedia");
  EXPECT_EQ(r[0].first_pub_year, 1320);

  /* delete */
  book dc_del;
  dc_del.id = r[0].id;
  dc_del.initialized = dc_del.f_id;
  st = dbm_.DeleteRows(dc_del);
  ASSERT_TRUE(is_status_ok(st));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
