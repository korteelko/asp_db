/**
 * asp_therm - implementation of real gas equations of state
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_queries_setup_select.h"

#include "db_tables.h"

namespace asp_db {
/* db_table_select_setup */
db_query_select_setup* db_query_select_setup::Init(const IDBTables* tables,
                                                   db_table _table) {
  return new db_query_select_setup(_table,
                                   *tables->GetFieldsCollection(_table));
}

void db_query_select_setup::ResetWhereClause(
    const std::shared_ptr<DBWhereClause<where_node_data> >& where) {
  where_ = where;
}

std::optional<std::string> db_query_select_setup::GetWhereString() const {
  return (where_.get() != nullptr)
             ? std::optional<std::string>{where_->GetString()}
             : std::nullopt;
}

db_query_select_setup::db_query_select_setup(
    db_table _table,
    const db_fields_collection& _fields)
    : db_query_basesetup(_table, _fields) {}

db_query_update_setup::db_query_update_setup(
    db_table _table,
    const db_fields_collection& _fields)
    : db_query_select_setup(_table, _fields) {}

/* db_table_select_result */
db_query_select_result::db_query_select_result(
    const db_query_select_setup& setup)
    : db_query_basesetup(setup) {}
}  // namespace asp_db
