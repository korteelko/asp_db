/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_connection_manager.h"

#include "db_connection_postgre.h"
#if defined(WITH_POSTGRESQL)
#  include "db_connection_postgre.h"
#endif  // WITH_POSTGRESQL
#include "Logging.h"

#include <ctime>

#include <assert.h>


namespace asp_db {
// db_parameters::db_parameters()
//   : supplier(db_client::NOONE) {}

Transaction::Transaction(DBConnection *connection)
  : status_(STATUS_DEFAULT), connection_(connection) {}

void Transaction::AddQuery(QuerySmartPtr &&query) {
  queries_.emplace_back(query);
}

mstatus_t Transaction::ExecuteQueries() {
  if (status_ == STATUS_DEFAULT) {
    for (auto it_query = queries_.begin(); it_query != queries_.end(); it_query++) {
      status_ = (*it_query)->Execute();
      // если статус после выполнения не удовлетворителен -
      //   откатим все изменения, залогируем ошибку
      if (!is_status_ok(status_)) {
        (*it_query)->LogDBConnectionError();
        auto ri = std::make_reverse_iterator(it_query);
        for (;ri != queries_.rend(); ++ri) {
          if ((*ri)->IsPerformed())
            (*ri)->unExecute();
        }
        break;
      }
    }
  }
  return status_;
}

/* DBException */
DBException::DBException(merror_t error, const std::string &msg)
  : error_(error, msg) {}
DBException::DBException(const std::string &msg)
  : DBException(ERROR_GENERAL_T, msg) {}

DBException &DBException::AddTableCode(db_table table) {
  table_ = table;
  return *this;
}

void DBException::LogException() {
  error_.LogIt();
}

merror_t DBException::GetError() const {
  return error_.GetErrorCode();
}

std::string DBException::GetErrorMessage() const {
  return error_.GetMessage();
}

mstatus_t DBConnectionManager::CheckConnection() {
  if (!(error_.GetErrorCode() && status_ == STATUS_HAVE_ERROR)) {
    if (!db_connection_)
      initDBConnection();
  }
  if (db_connection_ && is_status_aval(status_)) {
    Transaction tr(db_connection_.get());
    tr.AddQuery(QuerySmartPtr(
        new DBQuerySetupConnection(db_connection_.get())));
    tr.AddQuery(QuerySmartPtr(
        new DBQueryCloseConnection(db_connection_.get())));
    status_ = tryExecuteTransaction(tr);
  } else {
    error_.SetError(ERROR_DB_CONNECTION, "Не удалось установить"
        " соединение для БД: " + parameters_.GetInfo());
    status_ = STATUS_HAVE_ERROR;
  }
  return status_;
}

mstatus_t DBConnectionManager::ResetConnectionParameters(
    const db_parameters &parameters) {
  parameters_ = parameters;
  status_ = STATUS_DEFAULT;
  error_.Reset();
  return CheckConnection();
}

bool DBConnectionManager::IsTableExist(db_table dt) {
  bool exists = false;
  exec_wrap<db_table, bool, void (DBConnectionManager::*)(
      Transaction *, db_table, bool *)>(dt, &exists,
      &DBConnectionManager::isTableExist, nullptr);
  return exists;
}

mstatus_t DBConnectionManager::CreateTable(db_table dt) {
  db_save_point sp("create_table");
  return exec_wrap<db_table, void, void (DBConnectionManager::*)(
      Transaction *, db_table, void *)>(dt, nullptr,
      &DBConnectionManager::createTable, &sp);
}

bool DBConnectionManager::CheckTableFormat(db_table dt) {
  db_table_create_setup cs_db(dt);
  mstatus_t result = exec_wrap<db_table, db_table_create_setup, void (DBConnectionManager::*)(
      Transaction *, db_table, db_table_create_setup *)>(dt, &cs_db,
      &DBConnectionManager::getTableFormat, nullptr);

  if (is_status_ok(result)) {
    assert(0);
  }
  assert(0);

  return false;
}

mstatus_t DBConnectionManager::UpdateTableFormat(db_table dt) {
  db_table_create_setup cs_db(dt);
  mstatus_t result = exec_wrap<db_table, db_table_create_setup, void (DBConnectionManager::*)(
      Transaction *, db_table, db_table_create_setup *)>(dt, &cs_db,
      &DBConnectionManager::getTableFormat, nullptr);

  if (is_status_ok(result)) {
    assert(0);
  }
  assert(0);

  return result;
}


/*
mstatus_t DBConnectionManager::DeleteModelInfo(model_info &where) {
  std::unique_ptr<db_query_delete_setup> dds(
      db_query_delete_setup::Init(tables_, tables_->GetTableCode<model_info>()));
  if (dds)
    dds->where_condition.reset(tables_->InitWhereTree<model_info>(where));
  db_save_point sp("delete_rows");
  return exec_wrap<const db_query_delete_setup &, void,
      void (DBConnectionManager::*)(Transaction *, const db_query_delete_setup &,
      void *)>(*dds, nullptr, &DBConnectionManager::deleteRows, &sp);
}
*/

merror_t DBConnectionManager::GetErrorCode() {
  return error_.GetErrorCode();
}

mstatus_t DBConnectionManager::GetStatus() {
  return status_;
}

std::string DBConnectionManager::GetErrorMessage() {
  return error_.GetMessage();
}

DBConnectionManager::DBConnectionManager(const IDBTables *tables)
  : tables_(tables) {}

void DBConnectionManager::initDBConnection() {
  std::unique_lock<SharedMutex> lock(connect_init_lock_);
  status_ = STATUS_OK;
  try {
    db_connection_ = std::unique_ptr<DBConnection>(
        DBConnectionCreator().InitDBConnection(tables_, parameters_));
  } catch (DBException &e) {
    e.LogException();
    // если даже объект подключения был создан - затереть его
    db_connection_ = nullptr;
  }
  if (!db_connection_) {
    status_ = STATUS_HAVE_ERROR;
    error_.SetError(ERROR_DB_CONNECTION,
        "Подключение к базе данных не инициализировано");
  }
}

void DBConnectionManager::isTableExist(Transaction *tr,
    db_table dt, bool *is_exists) {
  tr->AddQuery(QuerySmartPtr(new DBQueryIsTableExists(
      db_connection_.get(), dt, *is_exists)));
}

void DBConnectionManager::createTable(Transaction *tr, db_table dt, void *) {
  tr->AddQuery(QuerySmartPtr(new DBQueryCreateTable(db_connection_.get(),
      tables_->CreateSetupByCode(dt))));
}

void DBConnectionManager::getTableFormat(Transaction *tr, db_table dt,
    db_table_create_setup *exist_table) {
  assert(0);
}

void DBConnectionManager::saveRows(Transaction *tr,
    const db_query_insert_setup &qi, id_container *id_vec) {
  tr->AddQuery(QuerySmartPtr(
      new DBQueryInsertRows(db_connection_.get(), qi, id_vec)));
}

void DBConnectionManager::selectRows(Transaction *tr,
    const db_query_select_setup &qs, db_query_select_result *result) {
  tr->AddQuery(QuerySmartPtr(
      new DBQuerySelectRows(db_connection_.get(), qs, result)));
}

void DBConnectionManager::deleteRows(Transaction *tr,
     const db_query_delete_setup &qd, void *) {
  tr->AddQuery(QuerySmartPtr(
      new DBQueryDeleteRows(db_connection_.get(), qd)));
}

mstatus_t DBConnectionManager::tryExecuteTransaction(Transaction &tr) {
  std::shared_lock<SharedMutex> lock(connect_init_lock_);
  mstatus_t trans_st;
  try {
    trans_st = tr.ExecuteQueries();
  } catch (const std::exception &e) {
    error_.SetError(ERROR_DB_CONNECTION, "Во время попытки "
        "подключения к БД перехвачено исключение: " + std::string(e.what()));
    error_.LogIt();
    trans_st = STATUS_HAVE_ERROR;
  }
  if (db_connection_) {
    if (db_connection_->IsOpen())
      db_connection_->CloseConnection();
    if (db_connection_->GetErrorCode())
      db_connection_->LogError();
  } else {
    if (!error_.GetErrorCode())
      error_.SetError(ERROR_DB_CONNECTION,
          "Подключение к базе данных не инициализировано");
    status_ = trans_st = STATUS_HAVE_ERROR;
  }
  return trans_st;
}

/* DBConnection::DBConnectionInstance */
DBConnectionManager::DBConnectionCreator::DBConnectionCreator() {}

DBConnection *DBConnectionManager::DBConnectionCreator::InitDBConnection(
    const IDBTables *tables, const db_parameters &parameters) {
  DBConnection *connect = nullptr;
  switch (parameters.supplier) {
    case db_client::NOONE:
      break;
  #if defined(WITH_POSTGRESQL)
    case db_client::POSTGRESQL:
      connect = new DBConnectionPostgre(tables, parameters);
      break;
  #endif  // WITH_POSTGRESQL
    // TODO: можно тут ошибку установить
    default:
      break;
  }
  return connect;
}
}  // namespace asp_db
