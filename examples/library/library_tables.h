/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * library_tables *
 *   Структура таблиц соответствующих структурам данных
 * library_structs.h. Также здесь прописан соответствующий
 * функционал сбора данных этих таблиц
 * ===================================================================
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef EXAMPLES__LIBRARY_TABLES_H
#define EXAMPLES__LIBRARY_TABLES_H

#include "asp_db/db_tables.h"
#include "library_structs.h"

#include <string>
#include <vector>

/* just example */

using namespace asp_db;

/* таблицы */
/* tables */
#define BOOK_TABLE 0x00010000
#define TRANSLATION_TABLE 0x00020000
#define AUTHOR_TABLE 0x00030000

/* cols */
/*   books */
/*|   id   |   title   |   first publ. year   |   orig. lang.|*/
/* UNIQUE(title, publ. year) */
#define BOOK_ID (BOOK_TABLE | 0x0001)
#define BOOK_TITLE (BOOK_TABLE | 0x0002)
#define BOOK_PUB_YEAR (BOOK_TABLE | 0x0003)
#define BOOK_LANG (BOOK_TABLE | 0x0004)

/*   translation */
/*|   id   |   book id   |   target lang.   |   target lang. title   |
 * translators| */
/* UNIQUE(id, lang, target lang. title) */
/* REFERENCE('book id' --> 'id' in table 'books') FOREIGN KEY */
#define TRANS_ID (TRANSLATION_TABLE | 0x0001)
#define TRANS_BOOK_ID (TRANSLATION_TABLE | 0x0002)
#define TRANS_LANG (TRANSLATION_TABLE | 0x0003)
#define TRANS_TRANS_TITLE (TRANSLATION_TABLE | 0x0004)
#define TRANS_TRANSLATORS (TRANSLATION_TABLE | 0x0005)

/*   author */
/*|   id   |   name   |   born year(NULL)   |   died year(NULL)   |
 * books[](NULL)| */
/* UNIQUE(name, born year, died year) */
#define AUTHOR_ID (AUTHOR_TABLE | 0x0001)
#define AUTHOR_NAME (AUTHOR_TABLE | 0x0002)
#define AUTHOR_BORN_YEAR (AUTHOR_TABLE | 0x0003)
#define AUTHOR_DIED_YEAR (AUTHOR_TABLE | 0x0004)
#define AUTHOR_BOOKS (AUTHOR_TABLE | 0x0005)

/* names */
/*   books */
#define BOOK_ID_NAME "book_id"
#define BOOK_TITLE_NAME "book_title"
#define BOOK_PUB_YEAR_NAME "book_pub_year"
#define BOOK_LANG_NAME "book_original_language"

/*   translation */
#define TRANS_ID_NAME "tr_id"
#define TRANS_BOOK_ID_NAME "tr_book_id"
#define TRANS_LANG_NAME "tr_language"
#define TRANS_TRANS_TITLE_NAME "tr_translated_name"
#define TRANS_TRANSLATORS_NAME "tr_translators"

/*   author */
#define AUTHOR_ID_NAME "aut_id"
#define AUTHOR_NAME_NAME "aut_name"
#define AUTHOR_BORN_YEAR_NAME "aut_born_year"
#define AUTHOR_DIED_YEAR_NAME "aut_died_year"
#define AUTHOR_BOOKS_NAME "aut_books"

enum library_db_tables {
  table_undefined = UNDEFINED_TABLE,
  table_book = BOOK_TABLE >> 16,
  table_translation = TRANSLATION_TABLE >> 16,
  table_author = AUTHOR_TABLE >> 16
};

/** \brief Перегруженные функции api БД */
class LibraryDBTables final : public IDBTables {
  std::string GetTablesNamespace() const override { return "LibraryDBTables"; }
  std::string GetTableName(db_table t) const override;
  const db_fields_collection* GetFieldsCollection(db_table t) const override;
  db_table StrToTableCode(const std::string& tname) const override;
  std::string GetIdColumnName(db_table dt) const override;
  const db_table_create_setup& CreateSetupByCode(db_table dt) const override;
};

/* Специализация шаблонов базового класса таблиц */
/* GetTableName */
template <>
std::string IDBTables::GetTableName<book>() const;
template <>
std::string IDBTables::GetTableName<translation>() const;
template <>
std::string IDBTables::GetTableName<author>() const;
/* GetTableCode */
template <>
db_table IDBTables::GetTableCode<book>() const;
template <>
db_table IDBTables::GetTableCode<translation>() const;
template <>
db_table IDBTables::GetTableCode<author>() const;

/* setInsertValues */
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам book */
template <>
void IDBTables::setInsertValues<book>(db_query_insert_setup* src,
                                      const book& select_data) const;
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам translation */
template <>
void IDBTables::setInsertValues<translation>(
    db_query_insert_setup* src,
    const translation& select_data) const;
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам author */
template <>
void IDBTables::setInsertValues<author>(db_query_insert_setup* src,
                                        const author& select_data) const;

/* SetSelectData */
/** \brief Записать в out_vec строки book из данных values_vec,
 *   полученных из БД
 * \note Обратная операция для db_query_insert_setup::setValues */
template <>
void IDBTables::SetSelectData<book>(db_query_select_result* src,
                                    std::vector<book>* out_vec) const;
/** \brief Записать в out_vec строки translation из данных values_vec,
 *   полученных из БД */
template <>
void IDBTables::SetSelectData<translation>(
    db_query_select_result* src,
    std::vector<translation>* out_vec) const;
/** \brief Записать в out_vec строки author из данных values_vec,
 *   полученных из БД */
template <>
void IDBTables::SetSelectData<author>(db_query_select_result* src,
                                      std::vector<author>* out_vec) const;

#endif  // !EXAMPLES__LIBRARY_TABLES_H
