/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "asp_db/db_connection.h"

#include "asp_db/db_connection_manager.h"

#include <map>
#include <sstream>

#include <assert.h>

namespace asp_db {
/* db_parameters */
db_parameters::db_parameters() : port(0), is_dry_run(true) {}

std::string db_parameters::GetInfo() const {
  std::string info = "Параметры базы данных:\n";
  if (is_dry_run)
    return info += "dummy connection\n";
  return info + db_client_to_string(supplier) + "\n\tname: " + name
         + "\n\tusername: " + username + "\n\thost: " + host + ":"
         + std::to_string(port) + "\n";
}

DBConnection::db_field_info::db_field_info(const std::string& name,
                                                  db_variable_type type)
    : name(name), type(type) {}

/* DBConnection */
DBConnection::DBConnection(const IDBTables* tables,
                           const db_parameters& parameters,
                           PrivateLogging* logger)
    : BaseObject(STATUS_DEFAULT),
      parameters_(parameters),
      tables_(tables),
      logger_(logger),
      is_connected_(false) {
  if (!tables_)
    throw DBException(ERROR_DB_CONNECTION,
                      "Попытка инициализовать DBConnection пустым "
                      "указатедем на функционал таблиц");
}

DBConnection::DBConnection(const DBConnection& r)
    : BaseObject(STATUS_DEFAULT),
      parameters_(r.parameters_),
      tables_(r.tables_),
      logger_(r.logger_) {}

DBConnection& DBConnection::operator=(const DBConnection& r) {
  if (&r != this) {
    // копируем
    parameters_ = db_parameters(r.parameters_);
    tables_ = r.tables_;
    logger_ = r.logger_;
    // установить дефолтные значения
    error_.Reset();
    status_ = STATUS_DEFAULT;
    is_connected_ = false;
  }
  return *this;
}

DBConnection::~DBConnection() {}

bool DBConnection::IsOpen() const {
  return is_connected_;
}

/* setup quries text */
std::stringstream DBConnection::setupAddSavePointString(
    const db_save_point& sp) {
  std::stringstream sstr;
  sstr << "SAVEPOINT " << sp.GetString() << ";";
  return sstr;
}
std::stringstream DBConnection::setupRollbackToSavePoint(
    const db_save_point& sp) {
  std::stringstream sstr;
  sstr << "ROLLBACK TO SAVEPOINT " << sp.GetString() << ";";
  return sstr;
}
std::stringstream DBConnection::setupAddColumnString(
    const std::pair<db_table, const db_variable&>& pdv) {
  std::stringstream sstr;
  sstr << "ALTER TABLE " << tables_->GetTableName(pdv.first) << " ADD COLUMN "
       << db_variable_to_string(pdv.second) << ";";
  return sstr;
}
std::stringstream DBConnection::setupCreateTableString(
    const db_table_create_setup& fields) {
  std::stringstream sstr;
  sstr << "CREATE TABLE " << tables_->GetTableName(fields.table) << " (";
  // сначала забить все поля
  for (const auto& field : fields.fields) {
    if (!error_.GetErrorCode()) {
      sstr << db_variable_to_string(field) << ", ";
    } else {
      break;
    }
  }
  if (!error_.GetErrorCode()) {
    // UNIQUE constraint
    sstr << db_unique_constrain_to_string(fields);
    // REFERENCES
    if (fields.ref_strings.get()) {
      for (const auto& ref : *fields.ref_strings) {
        sstr << db_reference_to_string(ref) << ", ";
        if (error_.GetErrorCode())
          break;
      }
    }
    // PRIMARY KEY
    if (!error_.GetErrorCode())
      sstr << db_primarykey_to_string(fields.pk_string);
  }
  sstr << ");";
  return sstr;
}
std::stringstream DBConnection::setupDropTableString(
    const db_table_drop_setup& drop) {
  std::stringstream sstr;
  sstr << "DROP TABLE " << tables_->GetTableName(drop.table) << " "
       << db_reference::GetReferenceActString(drop.act) << ";";
  return sstr;
}
std::stringstream DBConnection::setupInsertString(
    const db_query_insert_setup& fields) {
  if (fields.values_vec.empty()) {
    error_.SetError(ERROR_DB_VARIABLE, "Нет данных для INSERT операции");
    return std::stringstream();
  }
  std::string fnames =
      "INSERT INTO " + tables_->GetTableName(fields.table) + " (";
  std::string values = "VALUES (";
  for (const auto& row : fields.values_vec) {
    for (const auto& x : row) {
      if (x.first >= 0 && x.first < fields.fields.size()) {
        fnames += std::string(fields.fields[x.first].fname) + ", ";
        values += x.second + ", ";
      } else {
        Logging::Append(io_loglvl::debug_logs,
                        "Ошибка индекса операции INSERT.\n"
                        "\tДля таблицы "
                            + tables_->GetTableName(fields.table));
      }
    }
  }
  fnames.replace(fnames.size() - 2, fnames.size() - 1, ")");
  values.replace(values.size() - 2, values.size() - 1, ")");
  std::stringstream sstr;
  sstr << fnames << " " << values << " RETURNING ID;";
  return sstr;
}
std::stringstream DBConnection::setupDeleteString(
    const db_query_delete_setup& fields) {
  std::stringstream sstr;
  auto ws = fields.GetWhereString();
  if (ws != std::nullopt) {
    sstr << "DELETE FROM " << tables_->GetTableName(fields.table) << " WHERE "
         << ws.value() << ";";
  } else {
    sstr << "DELETE * FROM " << tables_->GetTableName(fields.table) << ";";
  }
  return sstr;
}  // namespace asp_db
std::stringstream DBConnection::setupSelectString(
    const db_query_select_setup& fields) {
  std::stringstream sstr;
  sstr << "SELECT * FROM " << tables_->GetTableName(fields.table);
  auto ws = fields.GetWhereString();
  if (ws != std::nullopt)
    sstr << " WHERE " << ws.value();
  sstr << ";";
  return sstr;
}
std::stringstream DBConnection::setupUpdateString(
    const db_query_update_setup& fields) {
  std::stringstream sstr;
  if (!fields.values.empty()) {
    sstr << "UPDATE " << tables_->GetTableName(fields.table) << " SET ";
    std::string set_str = "";
    for (const auto& x : fields.values)
      set_str +=
          std::string(fields.fields[x.first].fname) + " = " + x.second + ",";
    set_str[set_str.size() - 1] = ' ';
    sstr << set_str;
    auto ws = fields.GetWhereString();
    if (ws != std::nullopt)
      sstr << " WHERE " << ws.value();
    sstr << ";";
  }
  return sstr;
}

std::string DBConnection::db_unique_constrain_to_string(
    const db_table_create_setup& cs) {
  std::string result = "";
  for (const auto& x : cs.unique_constrains) {
    std::stringstream sstr;
    sstr << "UNIQUE(";
    for (const auto& y : x)
      sstr << y << ", ";
    result += sstr.str();
    result.replace(result.size() - 2, result.size() - 1, "),");
  }
  return result;
}

std::string DBConnection::db_reference_to_string(const db_reference& ref) {
  std::string str;
  merror_t ew = ref.CheckYourself();
  if (!ew) {
    if (ref.is_foreign_key)
      str += "FOREIGN KEY ";
    str += "(" + ref.fname + ") REFERENCES "
           + tables_->GetTableName(ref.foreign_table) + " (" + ref.foreign_fname
           + ")";
    if (ref.has_on_delete)
      str += " ON DELETE " + ref.GetReferenceActString(ref.delete_method);
    if (ref.has_on_update)
      str += " ON UPDATE " + ref.GetReferenceActString(ref.update_method);
  } else {
    error_.SetError(
        ew, "Проверка поля ссылки на другую таблицу завершилось ошибкой");
  }
  return str;
}

std::string DBConnection::db_primarykey_to_string(const db_complex_pk& pk) {
  std::string str;
  if (!pk.fnames.empty()) {
    str = "PRIMARY KEY (";
    for (const auto& fname : pk.fnames)
      str += fname + ", ";
    str.replace(str.size() - 2, str.size() - 1, ")");
  } else {
    error_.SetError(ERROR_DB_TABLE_PKEY,
                    "ни одного первичного ключа "
                    "для таблицы не задано");
  }
  return str;
}

bool DBConnection::isDryRun() {
  return parameters_.is_dry_run;
}

void DBConnection::passToLogger(io_loglvl ll,
                                const std::string& logger,
                                const std::string& msg) {
  if (logger_) {
    // зарегистрирован специальный логгер
    logger_->Append(ll, logger, msg);
  } else {
    // прокинуть на общий
    Logging::Append(ll, msg);
  }
}
}  // namespace asp_db
