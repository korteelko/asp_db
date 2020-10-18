/**
 * asp_db - db api of the project 'asp_therm'
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "library_tables.h"
#include "db_connection_manager.h"

#include <map>
#include <memory>

#include <assert.h>

#define tables_pair(x, y) \
  { x, y }

namespace table_fields_setup {
std::map<db_table, std::string> str_tables = {
    tables_pair(table_book, "book"),
    tables_pair(table_translation, "translation"),
    tables_pair(table_author, "author"),
};

/*
 * BOOKS
 */
/** \brief Сетап таблицы БД хранения данных о модели
 * \note В семантике PostgreSQL */
const db_fields_collection book_fields = {
    db_variable(TABLE_FIELD_PAIR(BOOK_ID),
                db_type::type_autoinc,
                {.is_primary_key = true, .can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(BOOK_TITLE),
                db_type::type_text,
                {.can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(BOOK_PUB_YEAR),
                db_type::type_int,
                {.can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(BOOK_LANG),
                db_type::type_int,
                {.can_be_null = false}),
};
static const db_table_create_setup::uniques_container book_uniques = {
    {TABLE_FIELD_NAME(BOOK_TITLE), TABLE_FIELD_NAME(BOOK_PUB_YEAR)}};
static const db_table_create_setup book_create_setup(table_book,
                                                     book_fields,
                                                     book_uniques,
                                                     nullptr);

/*
 * TRANSLATIONS
 */
const db_fields_collection translation_fields = {
    db_variable(TABLE_FIELD_PAIR(TRANS_ID),
                db_type::type_autoinc,
                {.is_primary_key = true, .can_be_null = false}),
    // reference to book(fk)
    db_variable(TABLE_FIELD_PAIR(TRANS_BOOK_ID),
                db_type::type_int,
                {.is_reference = true, .can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(TRANS_LANG),
                db_type::type_int,
                {.can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(TRANS_TRANS_TITLE),
                db_type::type_text,
                {.can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(TRANS_TRANSLATORS),
                db_type::type_text,
                {.can_be_null = false}),
};
static const db_table_create_setup::uniques_container tr_uniques = {
    {{TABLE_FIELD_NAME(TRANS_BOOK_ID), TABLE_FIELD_NAME(TRANS_LANG),
      TABLE_FIELD_NAME(TRANS_TRANS_TITLE)}}};
static const std::shared_ptr<db_ref_collection> translation_references(
    new db_ref_collection{db_reference(TABLE_FIELD_NAME(TRANS_BOOK_ID),
                                       table_book,
                                       TABLE_FIELD_NAME(BOOK_ID),
                                       true,
                                       db_reference_act::ref_act_cascade,
                                       db_reference_act::ref_act_cascade)});
static const db_table_create_setup translation_create_setup(
    table_translation,
    translation_fields,
    tr_uniques,
    translation_references);

/*
 * AUTHORS
 */
const db_fields_collection author_fields = {
    db_variable(TABLE_FIELD_PAIR(AUTHOR_ID),
                db_type::type_autoinc,
                {.is_primary_key = true, .can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(AUTHOR_NAME),
                db_type::type_text,
                {.can_be_null = false}),
    db_variable(TABLE_FIELD_PAIR(AUTHOR_BORN_YEAR), db_type::type_int, {}),
    db_variable(TABLE_FIELD_PAIR(AUTHOR_DIED_YEAR), db_type::type_int, {}),
    db_variable(TABLE_FIELD_PAIR(AUTHOR_BOOKS),
                db_type::type_text,
                {.is_array = true},
                0),
};
static const db_table_create_setup::uniques_container aut_uniques = {
    {{TABLE_FIELD_NAME(AUTHOR_NAME), TABLE_FIELD_NAME(AUTHOR_BORN_YEAR),
      TABLE_FIELD_NAME(AUTHOR_DIED_YEAR)}}};
static const db_table_create_setup author_create_setup(table_author,
                                                       author_fields,
                                                       aut_uniques,
                                                       nullptr);
}  // namespace table_fields_setup

namespace ns_tfs = table_fields_setup;

std::string LibraryDBTables::GetTableName(db_table t) const {
  auto x = ns_tfs::str_tables.find(t);
  return (x != ns_tfs::str_tables.end()) ? x->second : "";
}
const db_fields_collection* LibraryDBTables::GetFieldsCollection(
    db_table t) const {
  const db_fields_collection* result = nullptr;
  switch (t) {
    case table_book:
      result = &ns_tfs::book_fields;
      break;
    case table_translation:
      result = &ns_tfs::translation_fields;
      break;
    case table_author:
      result = &ns_tfs::author_fields;
      break;
    case table_undefined:
    default:
      throw DBException(ERROR_DB_TABLE_EXISTS, "Неизвестный код таблицы");
      break;
  }
  return result;
}
db_table LibraryDBTables::StrToTableCode(const std::string& tname) const {
  for (const auto& x : ns_tfs::str_tables)
    if (x.second == tname)
      return x.first;
  return table_undefined;
}
std::string LibraryDBTables::GetIdColumnName(db_table dt) const {
  std::string name = "";
  switch (dt) {
    case table_book:
      name = TABLE_FIELD_NAME(BOOK_ID);
      break;
    case table_translation:
      name = TABLE_FIELD_NAME(TRANS_ID);
      break;
    case table_author:
      name = TABLE_FIELD_NAME(AUTHOR_ID);
      break;
    case table_undefined:
      break;
  }
  return name;
}
const db_table_create_setup& LibraryDBTables::CreateSetupByCode(
    db_table dt) const {
  switch (dt) {
    case table_book:
      return ns_tfs::book_create_setup;
    case table_translation:
      return ns_tfs::translation_create_setup;
    case table_author:
      return ns_tfs::author_create_setup;
    case table_undefined:
    default:
      throw DBException(ERROR_DB_TABLE_EXISTS, "Неизвестный код таблицы");
  }
}

template <>
std::string IDBTables::GetTableName<book>() const {
  auto x = ns_tfs::str_tables.find(table_book);
  return (x != ns_tfs::str_tables.end()) ? x->second : "";
}

template <>
std::string IDBTables::GetTableName<translation>() const {
  auto x = ns_tfs::str_tables.find(table_translation);
  return (x != ns_tfs::str_tables.end()) ? x->second : "";
}

template <>
std::string IDBTables::GetTableName<author>() const {
  auto x = ns_tfs::str_tables.find(table_author);
  return (x != ns_tfs::str_tables.end()) ? x->second : "";
}

/* GetTableCode */
template <>
db_table IDBTables::GetTableCode<book>() const {
  return table_book;
}

template <>
db_table IDBTables::GetTableCode<translation>() const {
  return table_translation;
}

template <>
db_table IDBTables::GetTableCode<author>() const {
  return table_author;
}

/* setInsertValues */
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам book */
template <>
void IDBTables::setInsertValues<book>(db_query_insert_setup* src,
                                      const book& select_data) const {
  /* insert all data */
  db_query_basesetup::row_values values;
  db_query_basesetup::field_index i;
  if (select_data.id > 0)
    insert_macro(book::f_id, BOOK_ID, std::to_string(select_data.id));
  insert_macro(book::f_title, BOOK_TITLE, select_data.title);
  insert_macro(book::f_pub_year, BOOK_PUB_YEAR,
               std::to_string(select_data.first_pub_year));
  insert_macro(book::f_lang, BOOK_LANG, std::to_string(select_data.lang));
  src->values_vec.emplace_back(values);
}
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам translation */
template <>
void IDBTables::setInsertValues<translation>(
    db_query_insert_setup* src,
    const translation& select_data) const {
  db_query_basesetup::row_values values;
  db_query_basesetup::field_index i;
  if (select_data.id > 0)
    insert_macro(translation::f_id, TRANS_ID, std::to_string(select_data.id));
  if (select_data.book_p.first > 0)
    insert_macro(translation::f_book_p, TRANS_BOOK_ID,
                 std::to_string(select_data.book_p.first));
  insert_macro(translation::f_lang, TRANS_LANG,
               std::to_string(select_data.lang));
  insert_macro(translation::f_tr_name, TRANS_TRANS_TITLE,
               select_data.translated_name);
  insert_macro(translation::f_translators, TRANS_TRANSLATORS,
               select_data.translators);
  src->values_vec.emplace_back(values);
}
/** \brief Собрать вектор 'values' значений столбцов БД,
 *   по переданным строкам author */
template <>
void IDBTables::setInsertValues<author>(db_query_insert_setup* src,
                                        const author& select_data) const {
  db_query_basesetup::row_values values;
  db_query_basesetup::field_index i;
  if (select_data.id > 0)
    insert_macro(author::f_id, AUTHOR_ID, std::to_string(select_data.id));
  insert_macro(author::f_name, AUTHOR_NAME, select_data.name);
  insert_macro(author::f_b_year, AUTHOR_BORN_YEAR,
               std::to_string(select_data.born_year));
  insert_macro(author::f_d_year, AUTHOR_DIED_YEAR,
               std::to_string(select_data.died_year));
  if (select_data.books.size())
    insert_macro(author::f_books, AUTHOR_BOOKS,
                 db_variable::TranslateFromVector(select_data.books.begin(),
                                                  select_data.books.end()));
  src->values_vec.emplace_back(values);
}

std::string setInsertValue_author_book(const author& select_data) {
  return db_variable::TranslateFromVector(select_data.books.begin(),
                                          select_data.books.end());
}

/* SetSelectData */
/** \brief Записать в out_vec строки book из данных values_vec,
 *   полученных из БД
 * \note Обратная операция для db_query_insert_setup::setValues */
template <>
void IDBTables::SetSelectData<book>(db_query_select_result* src,
                                    std::vector<book>* out_vec) const {
  for (auto& row : src->values_vec) {
    book b;
    for (auto& col : row) {
      if (src->isFieldName(TABLE_FIELD_NAME(BOOK_ID), src->fields[col.first])) {
        b.id = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(BOOK_TITLE),
                                  src->fields[col.first])) {
        b.title = col.second.c_str();
      } else if (src->isFieldName(TABLE_FIELD_NAME(BOOK_PUB_YEAR),
                                  src->fields[col.first])) {
        b.first_pub_year = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(BOOK_LANG),
                                  src->fields[col.first])) {
        b.lang = (language_t)std::atoi(col.second.c_str());
      }
    }
    out_vec->push_back(std::move(b));
  }
}
/** \brief Записать в out_vec строки translation из данных values_vec,
 *   полученных из БД */
template <>
void IDBTables::SetSelectData<translation>(
    db_query_select_result* src,
    std::vector<translation>* out_vec) const {
  for (auto& row : src->values_vec) {
    translation tr;
    for (auto& col : row) {
      if (src->isFieldName(TABLE_FIELD_NAME(TRANS_ID),
                           src->fields[col.first])) {
        tr.id = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(TRANS_BOOK_ID),
                                  src->fields[col.first])) {
        tr.book_p.first = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(TRANS_LANG),
                                  src->fields[col.first])) {
        tr.lang = (language_t)std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(TRANS_TRANS_TITLE),
                                  src->fields[col.first])) {
        tr.translated_name = col.second.c_str();
      } else if (src->isFieldName(TABLE_FIELD_NAME(TRANS_TRANSLATORS),
                                  src->fields[col.first])) {
        tr.translators = col.second.c_str();
      }
    }
    out_vec->push_back(std::move(tr));
  }
}
/** \brief Записать в out_vec строки author из данных values_vec,
 *   полученных из БД */
template <>
void IDBTables::SetSelectData<author>(db_query_select_result* src,
                                      std::vector<author>* out_vec) const {
  for (auto& row : src->values_vec) {
    author a;
    for (auto& col : row) {
      if (src->isFieldName(TABLE_FIELD_NAME(AUTHOR_ID),
                           src->fields[col.first])) {
        a.id = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(AUTHOR_NAME),
                                  src->fields[col.first])) {
        a.name = col.second.c_str();
      } else if (src->isFieldName(TABLE_FIELD_NAME(AUTHOR_BORN_YEAR),
                                  src->fields[col.first])) {
        a.born_year = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(AUTHOR_DIED_YEAR),
                                  src->fields[col.first])) {
        a.died_year = std::atoi(col.second.c_str());
      } else if (src->isFieldName(TABLE_FIELD_NAME(AUTHOR_BOOKS),
                                  src->fields[col.first])) {
        string2Container(col.second, &a.books);
        /*
        std::vector<std::string> book_ids;
        col.second.erase(std::remove(col.second.begin(), col.second.end(), '['),
        col.second.end()); col.second.erase(std::remove(col.second.begin(),
        col.second.end(), ']'), col.second.end()); split_str(col.second,
        &book_ids, ','); if (!book_ids.empty()) for (const auto &x: book_ids)
            a.books.push_back(x);
      */
      }
    }
    out_vec->push_back(std::move(a));
  }
}
