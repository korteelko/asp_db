/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_query.h"

#include "Logging.h"
#include "db_connection.h"

namespace asp_db {
DBQuery::DBQuery(DBConnection* db_ptr)
    : status_(STATUS_DEFAULT), db_ptr_(db_ptr), is_performed_(false) {}

DBQuery::~DBQuery() {}

bool DBQuery::IsPerformed() const {
  return is_performed_;
}

void DBQuery::LogDBConnectionError() {
  if (db_ptr_)
    db_ptr_->LogError();
}

mstatus_t DBQuery::Execute() {
  if (db_ptr_) {
    if (status_ == STATUS_DEFAULT) {
      status_ = exec();
      is_performed_ = true;
    } else {
      if (IS_DEBUG_MODE)
        Logging::Append("not default status of query '" + q_info() + "'");
    }
  } else {
    status_ = STATUS_HAVE_ERROR;
    Logging::Append(ERROR_DB_QUERY_SETUP, "Execute setup connection query");
  }
  return status_;
}

void DBQuery::unExecute() {}

/* DBQuerySetupConnection */
DBQuerySetupConnection::DBQuerySetupConnection(DBConnection* db_ptr)
    : DBQuery(db_ptr) {}

void DBQuerySetupConnection::unExecute() {
  if (db_ptr_)
    db_ptr_->CloseConnection();
}

mstatus_t DBQuerySetupConnection::exec() {
  return db_ptr_->SetupConnection();
}

std::string DBQuerySetupConnection::q_info() {
  return "SetupConnection";
}

/* DBQueryCloseConnection */
DBQueryCloseConnection::DBQueryCloseConnection(DBConnection* db_ptr)
    : DBQuery(db_ptr) {}

mstatus_t DBQueryCloseConnection::Execute() {
  if (db_ptr_) {
    db_ptr_->CloseConnection();
    status_ = STATUS_OK;
  } else {
    status_ = STATUS_HAVE_ERROR;
    Logging::Append(ERROR_DB_QUERY_SETUP, "Execute close connection query");
  }
  return status_;
}

mstatus_t DBQueryCloseConnection::exec() {
  // метод Execute перегружен
  return STATUS_NOT;
}

std::string DBQueryCloseConnection::q_info() {
  return "CloseConnection";
}

/* DBQueryAddSavePoint */
DBQueryAddSavePoint::DBQueryAddSavePoint(DBConnection* ptr,
                                         const db_save_point& sp)
    : DBQuery(ptr), save_point(sp) {}

void DBQueryAddSavePoint::unExecute() {
  db_ptr_->RollbackToSavePoint(save_point);
}

mstatus_t DBQueryAddSavePoint::exec() {
  return db_ptr_->AddSavePoint(save_point);
}

std::string DBQueryAddSavePoint::q_info() {
  return "AddSavePoint";
}

/* DBQueryIsTableExist */
DBQueryIsTableExists::DBQueryIsTableExists(DBConnection* db_ptr,
                                           db_table dt,
                                           bool& is_exists)
    : DBQuery(db_ptr), table_(dt), is_exists_(is_exists) {}

mstatus_t DBQueryIsTableExists::exec() {
  return db_ptr_->IsTableExists(table_, &is_exists_);
}

std::string DBQueryIsTableExists::q_info() {
  return "IsTableExists";
}

/* DBQueryCreateTable */
DBQueryCreateTable::DBQueryCreateTable(DBConnection* db_ptr,
                                       const db_table_create_setup& setup)
    : DBQuery(db_ptr), create_setup(setup) {}

mstatus_t DBQueryCreateTable::exec() {
  return db_ptr_->CreateTable(create_setup);
}

std::string DBQueryCreateTable::q_info() {
  return "CreateTable";
}

/* DBQueryUpdateTable */
DBQueryUpdateTable::DBQueryUpdateTable(DBConnection* db_ptr,
                                       const db_table_create_setup& table_setup)
    : DBQuery(db_ptr), update_setup(table_setup) {}

mstatus_t DBQueryUpdateTable::exec() {
  return db_ptr_->UpdateTable(update_setup);
}

std::string DBQueryUpdateTable::q_info() {
  return "UpdateTable";
}

/* DBQueryInsertRows */
DBQueryInsertRows::DBQueryInsertRows(DBConnection* db_ptr,
                                     const db_query_insert_setup& insert_setup,
                                     id_container* id_vec)
    : DBQuery(db_ptr), insert_setup(insert_setup), id_vec(id_vec) {}

mstatus_t DBQueryInsertRows::exec() {
  return db_ptr_->InsertRows(insert_setup, id_vec);
}

std::string DBQueryInsertRows::q_info() {
  return "InsertRows";
}

/* DBQuerySelectRows */
DBQuerySelectRows::DBQuerySelectRows(DBConnection* db_ptr,
                                     const db_query_select_setup& select_setup,
                                     db_query_select_result* result)
    : DBQuery(db_ptr), select_setup(select_setup), result(result) {}

mstatus_t DBQuerySelectRows::exec() {
  return db_ptr_->SelectRows(select_setup, result);
}

std::string DBQuerySelectRows::q_info() {
  return "SelectRows";
}

/* DBQueryDeleteRows */
DBQueryDeleteRows::DBQueryDeleteRows(DBConnection* db_ptr,
                                     const db_query_delete_setup& delete_setup)
    : DBQuery(db_ptr), delete_setup(delete_setup) {}

mstatus_t DBQueryDeleteRows::exec() {
  return db_ptr_->DeleteRows(delete_setup);
}

std::string DBQueryDeleteRows::q_info() {
  return "DeleteRows";
}
}  // namespace asp_db
