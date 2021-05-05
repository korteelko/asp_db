/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * library_example main *
 * ===================================================================
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "asp_utils/Logging.h"
#include "db_connection.h"
#include "db_connection_manager.h"
#include "library_structs.h"
#include "library_tables.h"

#include <iostream>

#include <assert.h>

#define translation_construct(trans, i, l, bp, trs, ln, flags) \
  trans.id = i;                                                \
  trans.lang = l;                                              \
  trans.book_p = {bp.id, &bp};                                 \
  trans.translators = trs;                                     \
  trans.translated_name = ln;                                  \
  trans.initialized = flags;

#define author_construct(author, i, n, by, dy, flags, ...) \
  author.id = i;                                           \
  author.name = n;                                         \
  author.born_year = by;                                   \
  author.died_year = dy;                                   \
  author.books = {__VA_ARGS__};                            \
  author.initialized = flags;

using namespace asp_db;

db_parameters get_parameters() {
  db_parameters d;
  d.is_dry_run = false;
  d.supplier = db_client::POSTGRESQL;
  d.name = "labibliotecadebabel";
  d.username = "jorge";
  d.password = "my_pass";
  d.host = "127.0.0.1";
  d.port = 5432;
  return d;
}

int add_data(DBConnectionManager& dbm) {
  // книги
  // Данте
  book divine_comedy;
  book_construct(divine_comedy, -1, lang_ita, "Divina Commedia", 1320,
                 book::f_full & ~book::f_id);

  std::vector<book> books(4);
  // Сервантес
  book& quixote = books[0];
  book_construct(quixote, -1, lang_esp,
                 "El ingenioso hidalgo don Quijote de la Mancha", 1605,
                 book::f_full & ~book::f_id);
  // Лев Николаевич
  book& death_ii = books[1];
  book_construct(death_ii, -1, lang_rus, "Смерть Ивана Ильича", 1886,
                 book::f_full & ~book::f_id);
  book& resurrection = books[2];
  book_construct(resurrection, -1, lang_rus, "Воскресенье", 1899,
                 book::f_full & ~book::f_id);
  // Джойс
  book& ulysses = books[3];
  book_construct(ulysses, -1, lang_eng, "Ulysses", 1922,
                 book::f_full & ~book::f_id);

  book b;
  b.initialized = 0;
  // удалить все существующие данные
  std::vector<book> db_books;
  // get already exists books
  dbm.SelectAllRows(table_book, &db_books);
  for (const auto& x : db_books)
    Logging::Append(io_loglvl::info_logs, to_str(x));
  if (db_books.size())
    dbm.DeleteAllRows(table_book);
  mstatus_t st = dbm.SaveSingleRow(divine_comedy, &divine_comedy.id);
  if (is_status_ok(st)) {
    id_container ids;
    if (is_status_ok(dbm.SaveVectorOfRows(books, &ids))) {
      if (ids.id_vec.size() == books.size()) {
        for (size_t i = 0; i < books.size(); ++i)
          books[i].id = ids.id_vec[i];
      }
    }
  }

  // переводы
  std::vector<translation> trans(3);
  translation& divine_comedy_ru = trans[0];
  translation_construct(divine_comedy_ru, -1, lang_rus, divine_comedy,
                        "Лозинский М.Л.", "Божественная комедия",
                        translation::f_full & ~translation::f_id);
  translation& quixote_ru = trans[1];
  translation_construct(quixote_ru, -1, lang_rus, quixote, "Любимов Н.М.",
                        "Хитроумный идальго Дон Кихот Ламанчский",
                        translation::f_full & ~translation::f_id);
  translation& ulysses_ru = trans[2];
  translation_construct(ulysses_ru, -1, lang_rus, ulysses,
                        "Хинкис В.А., Хоружий С.С.", "Улисс",
                        translation::f_full & ~translation::f_id);
  dbm.DeleteAllRows(table_translation);
  st = dbm.SaveVectorOfRows(trans);

  // авторы
  std::vector<author> aths(4);
  author& dante = aths[0];
  author_construct(dante, -1, "Alighieri Dante", 1265, 1321,
                   author::f_full & ~author::f_id, divine_comedy.title);
  author& cervantes = aths[1];
  author_construct(cervantes, -1, "Miguel de Cervantes Saavedra", 1547, 1616,
                   author::f_full & ~author::f_id, quixote.title);
  author& tolstoy = aths[2];
  author_construct(tolstoy, -1, "Leo Tolstoy", 1828, 1910,
                   author::f_full & ~author::f_id, death_ii.title,
                   resurrection.title);
  author& joyce = aths[3];
  author_construct(joyce, -1, "James Augustine Aloysius Joyce", 1882, 1941,
                   author::f_full & ~author::f_id, ulysses.title);

  dbm.DeleteAllRows(table_author);
  st = dbm.SaveVectorOfRows(aths);

  /* select authors */
  std::vector<author> slt;

  LibraryDBTables ldb;
  WhereTreeConstructor<table_author> c(&ldb);
  WhereTree wt(c);
  wt.Init(c.Eq(AUTHOR_NAME, tolstoy.name));

  dbm.SelectRows(wt, &slt);
  if (slt.size()) {
    Logging::Append(io_loglvl::info_logs,
                    "В БД храниться информация о "
                    "нескольких книгах '"
                        + tolstoy.name + "'\n\t");
    for (const auto& book : slt[0].books)
      Logging::Append(io_loglvl::info_logs, book);
    Logging::Append(io_loglvl::info_logs, join_container(slt[0].books, ", "));
  } else {
    Logging::Append(
        io_loglvl::info_logs,
        "Не могу найти в таблице авторов писателя '" + tolstoy.name + "'");
  }

  return is_status_ok(st);
}

int test_table() {
  auto dp = get_parameters();
  LibraryDBTables lt;
  DBConnectionManager dbm(&lt);
  mstatus_t st = dbm.ResetConnectionParameters(dp);
  if (!is_status_ok(st)) {
    Logging::Append(io_loglvl::err_logs,
                    "reset connection error: " + dbm.GetErrorMessage());
    return -1;
  }
  std::vector<library_db_tables> tbs = {table_book, table_translation,
                                        table_author};
  for (const auto& t : tbs)
    if (!dbm.IsTableExists(t))
      dbm.CreateTable(t);
  if (is_status_ok(dbm.GetStatus())) {
    return add_data(dbm);
  } else {
    Logging::Append(io_loglvl::err_logs,
                    "Database table create error: " + dbm.GetErrorMessage());
    return -2;
  }
}

int main(int argc, char* argv[]) {
  Logging::InitDefault();
  return test_table();
}
