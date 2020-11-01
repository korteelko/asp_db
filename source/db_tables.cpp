/**
 * asp_therm - implementation of real gas equations of state
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_tables.h"

namespace asp_db {
/* idbtables_exception */
idbtables_exception::idbtables_exception(const std::string& msg)
    : tables_(nullptr), table_(UNDEFINED_TABLE), msg_(msg) {}
idbtables_exception::idbtables_exception(IDBTables* tables,
                                         const std::string& msg)
    : tables_(tables), table_(UNDEFINED_TABLE), msg_(msg) {}

idbtables_exception::idbtables_exception(class IDBTables* tables,
                                         db_table t,
                                         const std::string& msg)
    : tables_(tables), table_(t), msg_(msg) {}

idbtables_exception::idbtables_exception(class IDBTables* tables,
                                         db_table t,
                                         db_variable_id id,
                                         const std::string& msg)
    : tables_(tables), table_(t), field_id_(id), msg_(msg) {}

const char* idbtables_exception::what() const noexcept {
  return msg_.c_str();
}

std::string idbtables_exception::WhatWithDataInfo() const {
  std::string msg = msg_;
  if (tables_) {
    msg = "Table: " + tables_->GetTableName(table_) +
          "\nField id: " + std::to_string(field_id_) + "\nOriginal message: '" +
          msg_ + "'";
  }
  return msg;
}

db_table idbtables_exception::GetTable() const {
  return table_;
}

db_variable_id idbtables_exception::GetFieldID() const {
  return field_id_;
}

where_nodes_setup::node_ptr IDBTables::And(
    const where_nodes_setup::node_ptr& left,
    const where_nodes_setup::node_ptr& right) {
  return wns::node_and(left, right);
}

where_nodes_setup::node_ptr IDBTables::Or(
    const where_nodes_setup::node_ptr& left,
    const where_nodes_setup::node_ptr& right) {
  return wns::node_or(left, right);
}

}  // namespace asp_db
