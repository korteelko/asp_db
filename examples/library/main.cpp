/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * library_tables *
 *   Структура таблиц соответствующих структурам данных
 * library_structs.h. Также здесь прописан соответствующий
 * функционал сбора данных этих таблиц
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_connection.h"
#include "db_connection_manager.h"
#include "library_structs.h"
#include "library_tables.h"
#include "Logging.h"

#include <iostream>

#include <assert.h>


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

int add_data(DBConnectionManager &dbm) {
  // книги
  // Данте
  book divine_comedy;
  divine_comedy.id = -1;
  divine_comedy.lang = lang_ita;
  divine_comedy.title = "Divina Commedia";
  // дата завершения :)
  divine_comedy.first_pub_year = 1320;

  std::vector<book> books(4);
  // Сервантес
  book &quixote = books[0];
  quixote.id = -1;
  quixote.lang = lang_esp;
  quixote.title = "El ingenioso hidalgo don Quijote de la Mancha";
  quixote.first_pub_year = 1605;
  // Лев Николаевич
  book &death_ii = books[1];
  death_ii.id = -1;
  death_ii.lang = lang_rus;
  death_ii.title = "Смерть Ивана Ильича";
  death_ii.first_pub_year = 1886;
  book &resurrection = books[2];
  resurrection.id = -1;
  resurrection.lang = lang_rus;
  resurrection.title = "Воскресенье";
  resurrection.first_pub_year = 1899;
  // Джойс
  book &ulysses = books[3];
  ulysses.id = -1;
  ulysses.lang = lang_eng;
  ulysses.title = "Ulysses";
  ulysses.first_pub_year = 1922;

  book b;
  b.initialized = 1;
  dbm.DeleteRows(b);
  mstatus_t st = dbm.SaveSingleRow(divine_comedy, &divine_comedy.id);
  if (is_status_ok(st)) {
    id_container ids;
    dbm.SaveVectorOfRows(books, &ids);
    if (ids.id_vec.size() == books.size()) {
      for (size_t i = 0; i < books.size(); ++i)
        books[i].id = ids.id_vec[i];
    }
  }

  // переводы
  std::vector<translation> trans(3);
  translation &divine_comedy_ru = trans[0];
  divine_comedy_ru.id = -1;
  divine_comedy_ru.lang = lang_rus;
  divine_comedy_ru.book_p = {divine_comedy.id, &divine_comedy};
  divine_comedy_ru.translators = "Лозинский М.Л.";
  divine_comedy_ru.translated_name = "Божественная комедия";
  translation &quixote_ru = trans[1];
  quixote_ru.id = -1;
  quixote_ru.lang = lang_rus;
  quixote_ru.book_p = {quixote.id, &quixote};
  quixote_ru.translators = "Любимов Н.М.";
  quixote_ru.translated_name = "Хитроумный идальго Дон Кихот Ламанчский";
  translation &ulysses_ru = trans[2];
  ulysses_ru.id = -1;
  ulysses_ru.lang = lang_rus;
  ulysses_ru.book_p = {ulysses.id, &ulysses};
  ulysses_ru.translators = "Хинкис В.А., Хоружий С.С.";
  ulysses_ru.translated_name = "Улисс";

  translation t;
  t.initialized = 1;
  dbm.DeleteRows(t);
  st = dbm.SaveVectorOfRows(trans);

  // авторы
  std::vector<author> aths(4);
  author &dante = aths[0];
  dante.id = -1;
  dante.name = "Alighieri Dante";
  dante.born_year = 1265;
  dante.died_year = 1321;
  dante.books = {divine_comedy.title};
  author &cervantes = aths[1];
  cervantes.id = -1;
  cervantes.name = "Miguel de Cervantes Saavedra";
  cervantes.born_year = 1547;
  cervantes.died_year = 1616;
  cervantes.books = {quixote.title};
  author &tolstoy = aths[2];
  tolstoy.id = -1;
  tolstoy.name = "Leo Tolstoy";
  tolstoy.born_year = 1828;
  tolstoy.died_year = 1910;
  tolstoy.books = {
    death_ii.title,
    resurrection.title
  };
  author &joyce = aths[3];
  joyce.id = -1;
  joyce.name = "James Augustine Aloysius Joyce";
  joyce.born_year = 1882;
  joyce.died_year = 1941;
  joyce.books = {ulysses.title};

  author a;
  a.initialized = 1;
  dbm.DeleteRows(a);
  st = dbm.SaveVectorOfRows(aths);

  return is_status_ok(st);
}


int test_table() {
  auto dp = get_parameters();
  LibraryDBTables lt;
  DBConnectionManager dbm(&lt);
  mstatus_t st = dbm.ResetConnectionParameters(dp);
  if (!is_status_ok(st)) {
    std::cout << "reset connection error: " << dbm.GetErrorMessage();
    return -1;
  }
  std::vector<library_db_tables> tbs = {table_book,
      table_translation, table_author};
  for (const auto &t: tbs)
    if (!dbm.IsTableExists(t))
      dbm.CreateTable(t);
  if (is_status_ok(dbm.GetStatus())) {
    return add_data(dbm);
  } else {
    std::cout << "Database table create error: " << dbm.GetErrorMessage();
    return -2;
  }
}

int main(int argc, char *argv[]) {
  Logging::InitDefault();
  return test_table();
}
