/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_queries_setup.h"

#include "db_connection_manager.h"
#include "db_tables.h"
#include "Logging.h"

#include <algorithm>

#include <assert.h>


namespace asp_db {
/* db_save_point */
db_save_point::db_save_point(const std::string &_name)
  : name("") {
  std::string tmp = trim_str(_name);
  if (!tmp.empty()) {
    if (std::isalpha(tmp[0]) || tmp[0] == '_') {
      // проверить наличие пробелов и знаков разделения
      auto sep_pos = std::find_if_not(tmp.begin(), tmp.end(),
          [](char c) { return std::iswalnum(c) || c == '_'; });
      if (sep_pos == tmp.end())
        name = tmp;
    }
  }
  if (name.empty()) {
    Logging::Append(ERROR_DB_SAVE_POINT,
        "Ошибка инициализации параметров точки сохранения состояния БД\n"
        "Строка инициализации: " + _name);
  }
}

std::string db_save_point::GetString() const {
  return name;
}

/* db_table_create_setup */
db_table_create_setup::db_table_create_setup(db_table table)
  : table(table) {
  ref_strings = std::unique_ptr<db_ref_collection>(new db_ref_collection);
}

db_table_create_setup::db_table_create_setup(db_table table,
    const db_fields_collection &fields,
    const uniques_container &unique_constrains,
    const std::shared_ptr<db_ref_collection> &ref_strings)
  : table(table), fields(fields), unique_constrains(unique_constrains),
    ref_strings(ref_strings) {
  setupPrimaryKeyString();
}

void db_table_create_setup::setupPrimaryKeyString() {
  pk_string.fnames.clear();
  for (const auto &field: fields) {
    if (field.flags.is_primary_key)
      pk_string.fnames.push_back(field.fname);
  }
  if (pk_string.fnames.empty())
    throw DBException(ERROR_DB_TABLE_PKEY,
        "Пустой сетап элементов первичного ключа").AddTableCode(table);
}

void db_table_create_setup::CheckReferences(const IDBTables *tables) {
  if (ref_strings) {
    for (auto const &tref: *ref_strings) {
      // check fname present in fields
      bool exist = std::find_if(fields.begin(), fields.end(),
          [&tref](const db_variable &v)
              {return v.fname == tref.fname;}) != fields.end();
      if (exist) {
        const db_fields_collection *ffields =
            tables->GetFieldsCollection(tref.foreign_table);
        exist = std::find_if(ffields->begin(), ffields->end(),
            [&tref](const db_variable &v)
                {return v.fname == tref.foreign_fname;}) != ffields->end();
        if (!exist) {
          error.SetError(ERROR_DB_REFER_FIELD,
              "Неверное имя внешнего поля для reference.\n"
              "Таблица - " + tables->GetTableName(table) + "\n"
              "Внешняя таблица - " + tables->GetTableName(tref.foreign_table)
              + "\n" + STRING_DEBUG_INFO);
          break;
        }
      } else {
        error.SetError(ERROR_DB_REFER_FIELD,
            "Неверное собственное имя поля для reference");
        break;
      }
    }
  }
}

std::map<db_table_create_setup::compare_field, bool>
     db_table_create_setup::Compare(const db_table_create_setup &r) {
  std::map<db_table_create_setup::compare_field, bool> result;
  result.emplace(cf_table, r.table == table);
  // fields
  result.emplace(cf_fields, db_table_create_setup::IsSame(fields, r.fields));

  // pk_string
  result.emplace(cf_pk_string, db_table_create_setup::IsSame(
      pk_string.fnames, r.pk_string.fnames));

  // unique_constrains
  result.emplace(cf_unique_constrains, db_table_create_setup::IsSame(
      unique_constrains, r.unique_constrains));

  // ref_strings
  if (ref_strings && r.ref_strings)
    result.emplace(cf_unique_constrains, db_table_create_setup::IsSame(
        *ref_strings, *r.ref_strings));
  return result;
}

/* db_table_query_basesetup */
db_query_basesetup::db_query_basesetup(
    db_table table, const db_fields_collection &fields)
  : table(table), fields(fields) {}

db_query_basesetup::field_index db_query_basesetup::
    IndexByFieldId(db_variable_id fid) {
  field_index i = 0;
  for (auto const &x: fields) {
    if (x.fid == fid)
      break;
    ++i;
  }
  return (i == fields.size()) ? db_query_basesetup::field_index_end : i;
}

/* db_table_insert_setup */
db_query_insert_setup::db_query_insert_setup(db_table _table,
    const db_fields_collection &_fields)
  : db_query_basesetup(_table, _fields) {}


size_t db_query_insert_setup::RowsSize() const {
  return values_vec.size();
}

std::unique_ptr<db_where_tree> db_query_insert_setup::InitWhereTree() {
  if (values_vec.empty())
    return nullptr;
  // std::unique_ptr<db_where_tree> wt(new db_where_tree());
  std::shared_ptr<db_where_tree::condition_source> source(
      new db_where_tree::condition_source);
  auto &row = values_vec[0];
  for (const auto &x: row) {
    auto i = x.first;
    if (i != db_query_basesetup::field_index_end && i < fields.size()) {
      auto &f = fields[i];
      source->data.push_back(std::make_shared<db_condition_node>(
          f.type, f.fname, x.second));
    }
  }
  std::unique_ptr<db_where_tree> wt = std::make_unique<db_where_tree>(source);
  return wt;
}

/* db_table_select_setup */
db_query_select_setup *db_query_select_setup::Init(
    const IDBTables *tables, db_table _table) {
  return new db_query_select_setup(_table,
      *tables->GetFieldsCollection(_table));
}

db_query_select_setup::db_query_select_setup(db_table _table,
    const db_fields_collection &_fields)
  : db_query_basesetup(_table, _fields) {}

db_query_update_setup::db_query_update_setup(db_table _table,
    const db_fields_collection &_fields)
  : db_query_select_setup(_table, _fields) {}

/* db_table_select_result */
db_query_select_result::db_query_select_result(
    const db_query_select_setup &setup)
  : db_query_basesetup(setup) {}
}  // namespace asp_db
