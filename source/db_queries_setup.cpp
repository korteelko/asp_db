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

#include "Logging.h"
#include "db_connection_manager.h"
#include "db_tables.h"

#include <algorithm>

#include <assert.h>

namespace asp_db {
/* db_save_point */
db_save_point::db_save_point(const std::string& _name) : name("") {
  std::string tmp = trim_str(_name);
  if (!tmp.empty()) {
    if (std::isalpha(tmp[0]) || tmp[0] == '_') {
      // проверить наличие пробелов и знаков разделения
      auto sep_pos = std::find_if_not(tmp.begin(), tmp.end(), [](char c) {
        return std::iswalnum(c) || c == '_';
      });
      if (sep_pos == tmp.end())
        name = tmp;
    }
  }
  if (name.empty()) {
    Logging::Append(
        ERROR_DB_SAVE_POINT,
        "Ошибка инициализации параметров точки сохранения состояния БД\n"
        "Строка инициализации: " +
            _name);
  }
}

std::string db_save_point::GetString() const {
  return name;
}

/* db_table_create_setup */
db_table_create_setup::db_table_create_setup(db_table table)
    : table(table),
      ref_strings(std::unique_ptr<db_ref_collection>(new db_ref_collection())) {
}

db_table_create_setup::db_table_create_setup(
    db_table table,
    const db_fields_collection& fields,
    const uniques_container& unique_constrains,
    const std::shared_ptr<db_ref_collection>& ref_strings)
    : table(table),
      fields(fields),
      unique_constrains(unique_constrains),
      ref_strings(ref_strings) {
  setupPrimaryKeyString();
}

void db_table_create_setup::setupPrimaryKeyString() {
  pk_string.fnames.clear();
  for (const auto& field : fields) {
    if (field.flags.is_primary_key)
      pk_string.fnames.push_back(field.fname);
  }
  if (pk_string.fnames.empty())
    throw DBException(ERROR_DB_TABLE_PKEY,
                      "Пустой сетап элементов первичного ключа")
        .AddTableCode(table);
}

void db_table_create_setup::CheckReferences(const IDBTables* tables) {
  if (ref_strings) {
    for (auto const& tref : *ref_strings) {
      // check fname present in fields
      bool exist = std::find_if(fields.begin(), fields.end(),
                                [&tref](const db_variable& v) {
                                  return v.fname == tref.fname;
                                }) != fields.end();
      if (exist) {
        const db_fields_collection* ffields =
            tables->GetFieldsCollection(tref.foreign_table);
        exist = std::find_if(ffields->begin(), ffields->end(),
                             [&tref](const db_variable& v) {
                               return v.fname == tref.foreign_fname;
                             }) != ffields->end();
        if (!exist) {
          error.SetError(ERROR_DB_REFER_FIELD,
                         "Неверное имя внешнего поля для reference.\n"
                         "Таблица - " +
                             tables->GetTableName(table) +
                             "\n"
                             "Внешняя таблица - " +
                             tables->GetTableName(tref.foreign_table) + "\n" +
                             STRING_DEBUG_INFO);
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
db_table_create_setup::Compare(const db_table_create_setup& r) {
  std::map<db_table_create_setup::compare_field, bool> result;
  result.emplace(cf_table, r.table == table);
  // fields
  result.emplace(cf_fields, db_table_create_setup::IsSame(fields, r.fields));

  // pk_string
  result.emplace(cf_pk_string, db_table_create_setup::IsSame(
                                   pk_string.fnames, r.pk_string.fnames));

  // unique_constrains
  result.emplace(
      cf_unique_constrains,
      db_table_create_setup::IsSame(unique_constrains, r.unique_constrains));

  // ref_strings
  if (ref_strings && r.ref_strings)
    result.emplace(cf_unique_constrains,
                   db_table_create_setup::IsSame(*ref_strings, *r.ref_strings));
  return result;
}

/* db_table_query_basesetup */
db_query_basesetup::db_query_basesetup(db_table table,
                                       const db_fields_collection& fields)
    : table(table), fields(fields) {}

db_query_basesetup::field_index db_query_basesetup::IndexByFieldId(
    db_variable_id fid) {
  field_index i = 0;
  for (auto const& x : fields) {
    if (x.fid == fid)
      break;
    ++i;
  }
  return (i == fields.size()) ? db_query_basesetup::field_index_end : i;
}

/* db_table_insert_setup */
db_query_insert_setup::db_query_insert_setup(
    db_table _table,
    const db_fields_collection& _fields)
    : db_query_basesetup(_table, _fields) {}

std::shared_ptr<DBWhereClause<where_node_data>>
db_query_insert_setup::InitInsertTree() {
  if (values_vec.empty())
    return nullptr;
  std::shared_ptr<DBWhereClause<where_node_data>> clause = nullptr;
  std::vector<std::shared_ptr<expression_node<where_node_data>>> source;
  source.reserve(values_vec[0].size());
  const auto& row = values_vec[0];
  for (const auto& x : row) {
    // инициализировать все 3х элементные поддеревья типа
    //             EQ
    //            /  \       -->   `$fname EQ $value`
    //        fname  value
    // и записать их в контейнер
    auto i = x.first;
    if (i != db_query_basesetup::field_index_end && i < fields.size()) {
      auto& f = fields[i];
      auto node = where_node_creator<db_operator_t::op_eq>::create(
          std::string(f.fname), where_table_pair(f.type, x.second));
      if (node.get())
        source.push_back(node);
    }
  }
  if (source.size()) {
    // соединить все 3х элементные поддеревья оператором AND в одно большое
    // дерево
    clause = std::make_shared<DBWhereClause<where_node_data>>(source[0]);
    for (auto it = source.begin() + 1; it != source.end(); ++it)
      clause->AddCondition(db_operator_wrapper(db_operator_t::op_and), *it);
  }
  return clause;
}
}  // namespace asp_db
