
/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "asp_db/db_connection_firebird.h"
#include "asp_db/db_connection_manager.h"

namespace asp_db {
#define types_pair(x, y) \
  { x, y }
IMaster* DBConnectionFireBird::_firebird_work::master =
    fb_get_master_interface();

namespace firebird_impl {
/** \brief Мапа соответствий типов данных db_variable_type библиотеки
 *   с FireBird типами данных */
static std::map<db_variable_type, std::string> str_db_variable_types =
    std::map<db_variable_type, std::string>{
        types_pair(db_variable_type::type_empty, ""),
        types_pair(db_variable_type::type_autoinc, "BIGINT"),
        types_pair(db_variable_type::type_bool, "BOOL"),
        types_pair(db_variable_type::type_int, "INTEGER"),
        types_pair(db_variable_type::type_long, "BIGINT"),
        types_pair(db_variable_type::type_real, "FLOAT"),
        types_pair(db_variable_type::type_date, "DATE"),
        types_pair(db_variable_type::type_time, "TIME"),
        types_pair(db_variable_type::type_char_array, "CHAR"),
        types_pair(db_variable_type::type_text, "CHAR VARYING"),
    };
/** \brief Найти соответствующий строковому представлению
 *   тип данных приложения
 * \todo Получается весьма тонкое место,
 *   из-за специализации времени например */
db_variable_type find_type(const std::string& uppercase_str) {
  for (const auto& x : str_db_variable_types)
    if (x.second == uppercase_str)
      return x.first;
  /* Несколько специфичные типы */
  /* postgres выдаёт строку с широким разворотом по времени,включая
   *   инфо по временному поясу так что это надо расширять */
  std::size_t find_pos = uppercase_str.find("TIME");
  if (find_pos != std::string::npos)
    return db_variable_type::type_time;
  /* про соотношение character/text не знаю что делать */
  return db_variable_type::type_empty;
}
/**
 * \brief Функтор для сетапа полей деревьев условий для
 *   PostgreSQL СУБД
 * */
struct where_string_set {
  /**
   * \brief Преобразование данных поля к читаемому postgres
   * */
  std::string operator()(db_variable_type t, const std::string& v) { return v; }
};

/** \brief Распарсить строку достать из неё все целые числа */
void StringToIntNumbers(const std::string& str, std::vector<int>* result) {
  std::string num = "";
  auto it = str.begin();
  while (it != str.end()) {
    if (std::isdigit(*it)) {
      num += *it;
    } else {
      if (!num.empty()) {
        int n = atoi(num.c_str());
        // полученные индексы начинаются с 1, не с 0
        result->push_back(n - 1);
        num = "";
      }
    }
    ++it;
  }
}
}  // namespace firebird_impl

DBConnectionFireBird::DBConnectionFireBird(const IDBTables* tables,
                                           const db_parameters& parameters,
                                           PrivateLogging* logger)
    : DBConnection(tables, parameters, logger), firebird_work(parameters) {}

DBConnectionFireBird::DBConnectionFireBird(const DBConnectionFireBird& r)
    : DBConnection(r), firebird_work(r.parameters_) {}

DBConnectionFireBird& DBConnectionFireBird::operator=(
    const DBConnectionFireBird& r) {
  if (&r != this) {
    // копируем
    parameters_ = r.parameters_;
    tables_ = r.tables_;
    logger_ = r.logger_;
    // установить дефолтные значения
    error_.Reset();
    status_ = STATUS_DEFAULT;
    is_connected_ = false;
    // firebird_work.ReleaseConnection();
  }
  return *this;
}

DBConnectionFireBird::~DBConnectionFireBird() {
  CloseConnection();
}

std::shared_ptr<DBConnection> DBConnectionFireBird::CloneConnection() {
  return std::shared_ptr<DBConnectionFireBird>(new DBConnectionFireBird(*this));
}

mstatus_t DBConnectionFireBird::AddSavePoint(const db_save_point& sp) {
  return exec_wrap<
      db_save_point, void,
      std::stringstream (DBConnectionFireBird::*)(const db_save_point&),
      void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      sp, nullptr, &DBConnectionFireBird::setupAddSavePointString,
      &DBConnectionFireBird::execAddSavePoint);
}

void DBConnectionFireBird::RollbackToSavePoint(const db_save_point& sp) {
  exec_wrap<db_save_point, void,
            std::stringstream (DBConnectionFireBird::*)(const db_save_point&),
            void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      sp, nullptr, &DBConnectionFireBird::setupRollbackToSavePoint,
      &DBConnectionFireBird::execRollbackToSavePoint);
}

mstatus_t DBConnectionFireBird::SetupConnection() {
  if (!isDryRun()) {
    try {
      firebird_work.InitConnection();
    } catch (const std::exception& e) {
      error_.SetError(ERROR_DB_CONNECTION,
                      "Подключение к БД: exception. Запрос:\n"
                          + parameters_.GetInfo()
                          + "\nexception what: " + e.what());
      status_ = STATUS_HAVE_ERROR;
    }
  } else {
    status_ = STATUS_OK;
    passToLogger(io_loglvl::info_logs, FIREBIRD_DRYRUN_LOGGER,
                 "dry_run connect:" + parameters_.GetInfo());
    passToLogger(io_loglvl::info_logs, FIREBIRD_DRYRUN_LOGGER,
                 "dry_run transaction begin");
  }
  return status_;
}

void DBConnectionFireBird::CloseConnection() {
  // если собирали транзакцию - закрыть
  if (firebird_work.IsAvailable())
    firebird_work.commit_detach();
  // fuuuuuuuuu
  firebird_work.ReleaseConnection();
  is_connected_ = false;
  error_.Reset();
  if (IS_DEBUG_MODE)
    Logging::Append(io_loglvl::debug_logs,
                    "Закрытие соединения c БД " + parameters_.name);
  if (isDryRun()) {
    status_ = STATUS_OK;
    passToLogger(io_loglvl::info_logs, FIREBIRD_DRYRUN_LOGGER,
                 "dry_run commit and disconect");
  }
}

mstatus_t DBConnectionFireBird::IsTableExists(db_table t, bool* is_exists) {
  return exec_wrap<
      db_table, bool, std::stringstream (DBConnectionFireBird::*)(db_table),
      void (DBConnectionFireBird::*)(const std::stringstream&, bool*)>(
      t, is_exists, &DBConnectionFireBird::setupTableExistsString,
      &DBConnectionFireBird::execIsTableExists);
}

mstatus_t DBConnectionFireBird::GetTableFormat(db_table t,
                                               db_table_create_setup* fields) {
  assert(0 && "не оттестированно");
  return STATUS_NOT;
}

mstatus_t DBConnectionFireBird::CheckTableFormat(
    const db_table_create_setup& fields) {
  /* todo */
  assert(0);
  return STATUS_NOT;
}

mstatus_t DBConnectionFireBird::UpdateTable(
    const db_table_create_setup& fields) {
  /* todo */
  assert(0);
  return STATUS_NOT;
}

mstatus_t DBConnectionFireBird::CreateTable(
    const db_table_create_setup& fields) {
  return exec_wrap<
      db_table_create_setup, void,
      std::stringstream (DBConnectionFireBird::*)(const db_table_create_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      fields, nullptr, &DBConnectionFireBird::setupCreateTableString,
      &DBConnectionFireBird::execCreateTable);
}

mstatus_t DBConnectionFireBird::DropTable(const db_table_drop_setup& drop) {
  return exec_wrap<
      db_table_drop_setup, void,
      std::stringstream (DBConnectionFireBird::*)(const db_table_drop_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      drop, nullptr, &DBConnectionFireBird::setupDropTableString,
      &DBConnectionFireBird::execDropTable);
}

mstatus_t DBConnectionFireBird::InsertRows(
    const db_query_insert_setup& insert_data,
    id_container* id_vec) {
  mstatus_t status = exec_wrap<
      db_query_insert_setup, metadata_t,
      std::stringstream (DBConnectionFireBird::*)(const db_query_insert_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, metadata_t*)>(
      insert_data, nullptr, &DBConnectionFireBird::setupInsertString,
      &DBConnectionFireBird::execInsert);
  return status;
}

mstatus_t DBConnectionFireBird::DeleteRows(
    const db_query_delete_setup& delete_data) {
  return exec_wrap<
      db_query_delete_setup, void,
      std::stringstream (DBConnectionFireBird::*)(const db_query_delete_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      delete_data, nullptr, &DBConnectionFireBird::setupDeleteString,
      &DBConnectionFireBird::execDelete);
}

mstatus_t DBConnectionFireBird::SelectRows(
    const db_query_select_setup& select_data,
    db_query_select_result* result_data) {
  metadata_t result;
  result_data->values_vec.clear();
  mstatus_t res = exec_wrap<
      db_query_select_setup, metadata_t,
      std::stringstream (DBConnectionFireBird::*)(const db_query_select_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, metadata_t*)>(
      select_data, &result, &DBConnectionFireBird::setupSelectString,
      &DBConnectionFireBird::execSelect);
  return res;
}

mstatus_t DBConnectionFireBird::UpdateRows(
    const db_query_update_setup& update_data) {
  return exec_wrap<
      db_query_update_setup, void,
      std::stringstream (DBConnectionFireBird::*)(const db_query_update_setup&),
      void (DBConnectionFireBird::*)(const std::stringstream&, void*)>(
      update_data, nullptr, &DBConnectionFireBird::setupUpdateString,
      &DBConnectionFireBird::execUpdate);
}

std::stringstream DBConnectionFireBird::setupTableExistsString(db_table t) {
  std::stringstream select_ss;
  select_ss << "SELECT 1 FROM RDB$RELATIONS WHERE RDB$RELATION_NAME = "
            << tables_->GetTableName(t) << ";";
  return select_ss;
}
std::stringstream DBConnectionFireBird::setupGetColumnsInfoString(db_table t) {
  assert(0);
  std::stringstream select_ss;
  return select_ss;
}
std::stringstream DBConnectionFireBird::setupGetConstrainsString(db_table t) {
  assert(0);
  std::stringstream sstr;
  return sstr;
}

std::stringstream DBConnectionFireBird::setupGetForeignKeys(db_table t) {
  assert(0);
  std::stringstream sstr;
  return sstr;
}

std::stringstream DBConnectionFireBird::setupInsertString(
    const db_query_insert_setup& fields) {
  if (fields.values_vec.empty()) {
    error_.SetError(ERROR_DB_VARIABLE, "Нет данных для INSERT операции");
    return std::stringstream();
  }
  if (fields.values_vec[0].empty()) {
    error_.SetError(ERROR_DB_VARIABLE, "INSERT операция для пустых строк");
    return std::stringstream();
  }
  std::string fnames =
      "INSERT INTO " + tables_->GetTableName(fields.table) + " (";
  std::vector<std::string> vals;
  // set fields
  auto& row_values = fields.values_vec[0];
  for (auto& x : row_values)
    fnames += std::string(fields.fields[x.first].fname) + ", ";
  fnames.replace(fnames.size() - 2, fnames.size() - 1, ")");

  for (const auto& row : fields.values_vec) {
    std::string value = " (";
    for (const auto& x : row) {
      if (x.first >= 0 && (x.first < fields.fields.size())) {
        addVariableToString(&value, fields.fields[x.first], x.second);
      } else {
        Logging::Append(io_loglvl::debug_logs,
                        "Ошибка индекса операции INSERT.\n"
                        "\tДля таблицы "
                            + tables_->GetTableName(fields.table));
      }
    }
    value.replace(value.size() - 2, value.size(), "),");
    vals.emplace_back(value);
  }
  vals.back().replace(vals.back().size() - 1, vals.back().size(), " ");
  std::stringstream sstr;
  sstr << fnames << " VALUES ";
  for (const auto& x : vals)
    sstr << x;
  sstr << ";";
  return sstr;
}

std::stringstream DBConnectionFireBird::setupDeleteString(
    const db_query_delete_setup& fields) {
  std::stringstream sstr;
  firebird_impl::where_string_set ws();
  sstr << "DELETE FROM " << tables_->GetTableName(fields.table);
  auto wstr = fields.GetWhereString();
  if (wstr != std::nullopt)
    sstr << " WHERE " << wstr.value();
  sstr << ";";
  return sstr;
}
std::stringstream DBConnectionFireBird::setupSelectString(
    const db_query_select_setup& fields) {
  std::stringstream sstr;
  firebird_impl::where_string_set ws();
  sstr << "SELECT * FROM " << tables_->GetTableName(fields.table);
  auto wstr = fields.GetWhereString();
  if (wstr != std::nullopt)
    sstr << " WHERE " << wstr.value();
  sstr << ";";
  return sstr;
}
std::stringstream DBConnectionFireBird::setupUpdateString(
    const db_query_update_setup& fields) {
  firebird_impl::where_string_set ws();
  std::stringstream sstr;
  if (!fields.values.empty()) {
    sstr << "UPDATE " << tables_->GetTableName(fields.table) << " SET ";
    std::string set_str = "";
    for (const auto& x : fields.values)
      set_str +=
          std::string(fields.fields[x.first].fname) + " = " + x.second + ",";
    set_str[set_str.size() - 1] = ' ';
    sstr << set_str;
    auto wstr = fields.GetWhereString();
    if (wstr != std::nullopt)
      sstr << " WHERE " << wstr.value();
    sstr << ";";
  }
  return sstr;
}

void DBConnectionFireBird::execWithoutReturn(const std::stringstream& sstr) {
  firebird_work.execute(sstr);
}
void DBConnectionFireBird::execWithReturn(const std::stringstream& sstr,
                                          metadata_t* result) {
  firebird_work.prepare(sstr);
  firebird_work.get_result(*result);
}

void DBConnectionFireBird::execAddSavePoint(const std::stringstream& sstr,
                                            void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execRollbackToSavePoint(
    const std::stringstream& sstr,
    void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execIsTableExists(const std::stringstream& sstr,
                                             bool* is_exists) {
  metadata_t result;
  execWithReturn(sstr, &result);
}
void DBConnectionFireBird::execAddColumn(const std::stringstream& sstr, void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execCreateTable(const std::stringstream& sstr,
                                           void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execDropTable(const std::stringstream& sstr, void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execInsert(const std::stringstream& sstr,
                                      metadata_t* result) {
  execWithReturn(sstr, result);
}
void DBConnectionFireBird::execDelete(const std::stringstream& sstr, void*) {
  execWithoutReturn(sstr);
}
void DBConnectionFireBird::execSelect(const std::stringstream& sstr,
                                      metadata_t* result) {
  execWithReturn(sstr, result);
}
void DBConnectionFireBird::execUpdate(const std::stringstream& sstr, void*) {
  execWithoutReturn(sstr);
}

merror_t DBConnectionFireBird::setConstrainVector(
    const std::vector<int>& indexes,
    const db_fields_collection& fields,
    std::vector<std::string>* output) {
  merror_t error = ERROR_SUCCESS_T;
  output->clear();
  for (const size_t i : indexes) {
    if (i < fields.size()) {
      output->push_back(fields[i].fname);
    } else {
      error = error_.SetError(
          ERROR_DB_TABLE_PKEY,
          "Ошибка создания первичного ключа по параметрам из БД: "
          "индекс столбца вне границ таблицы");
    }
  }
  return error;
}

void DBConnectionFireBird::addVariableToString(std::string* str_p,
                                               const db_variable& var,
                                               const std::string& value) {
  db_variable_type t = var.type;
  if (var.flags.is_array && t != db_variable_type::type_char_array) {
    std::string str = "ARRAY[";
    std::vector<std::string> vec;
    vector_wrapper n(vec);
    if (is_status_ok(db_variable::TranslateToVector(value, AppendOp(n)))) {
      for (const auto& x : vec)
        str += getVariableValue(var, x);
      size_t lp = str.rfind(',');
      if (lp != std::string::npos)
        str[lp] = ' ';
    }
    str += "], ";
    *str_p += str;
  } else {
    *str_p += getVariableValue(var, value);
  }
}

std::string DBConnectionFireBird::getVariableValue(const db_variable& var,
                                                   const std::string& value) {
  db_variable_type t = var.type;
  std::string str = "";
  if ((t == db_variable_type::type_char_array
       || t == db_variable_type::type_text)) {
    str = value + ", ";
  } else {
    str += value + ", ";
  }
  return str;
}

std::string DBConnectionFireBird::getOnExistActForInsert(
    insert_on_exists_act act) {
  switch (act) {
    case insert_on_exists_act::do_nothing:
      return " ON CONFLICT DO NOTHING ";
    case insert_on_exists_act::do_update:
      return " ON CONFLICT DO UPDATE ";
    case insert_on_exists_act::not_set:
    default:
      return " ";
  }
  throw DBException(
      ERROR_DB_QUERY_SETUP,
      "Неизвестный тип действия для `ON CONFLICT` условия postgres");
}

std::string DBConnectionFireBird::db_variable_to_string(const db_variable& dv) {
  std::stringstream ss;
  merror_t ew = dv.CheckYourself();
  if (!ew) {
    ss << dv.fname << " ";
    auto itDBtype = firebird_impl::str_db_variable_types.find(dv.type);
    if (itDBtype != firebird_impl::str_db_variable_types.end()) {
      // todo: вроде как здесь пример для массива символов
      //   в обычном массиве скобки квадратны, и пусты могут быть
      // алсо, как насчёт массива массивов
      ss << firebird_impl::str_db_variable_types[dv.type];
      if (dv.flags.is_array) {
        // если dv.len == 0, то массив не ограничен(так можно)
        std::string len_str = (dv.len > 0) ? std::to_string(dv.len) : "";
        if (dv.type == db_variable_type::type_char_array)
          ss << "(" << len_str << ")";
        else
          ss << "[" << len_str << "]";
      }
      if (!dv.flags.can_be_null)
        ss << " NOT NULL";
      if (dv.flags.has_default)
        ss << " DEFAULT " << dv.default_str;
    } else {
      error_.SetError(ERROR_DB_VARIABLE,
                      "Тип переменной не задан для данной имплементации БД");
    }
  }  // namespace asp_db
  else {
    error_.SetError(ew, "Проверка параметров поля таблицы завершилось ошибкой");
  }
  return ss.str();
}

// _firebird_work
DBConnectionFireBird::_firebird_work::_firebird_work(
    const db_parameters& parameters)
    : BaseObject(STATUS_DEFAULT), parameters(parameters) {
  if (master != nullptr) {
    st = master->getStatus();
    prov = master->getDispatcher();
    utl = master->getUtilInterface();
    // fb_status = std::make_unique<ThrowStatusWrapper>(st);
    fb_status = std::unique_ptr<ThrowStatusWrapper>(new ThrowStatusWrapper(st));
    status_ = STATUS_OK;
  } else {
    status_ = STATUS_HAVE_ERROR;
    error_.SetError(ERROR_DB_CONNECTION, "Инициализация мастера соединения");
  }
}

DBConnectionFireBird::_firebird_work::~_firebird_work() {
  ReleaseConnection();
}

bool DBConnectionFireBird::_firebird_work::InitConnection(bool read_only) {
  try {
    if (!read_only) {
      dpb = utl->getXpbBuilder(fb_status.get(), IXpbBuilder::DPB, NULL, 0);
      att = prov->attachDatabase(fb_status.get(), parameters.name.c_str(), 0,
                                 NULL);
    } else {
      dpb = utl->getXpbBuilder(fb_status.get(), IXpbBuilder::TPB, NULL, 0);
      dpb->insertTag(fb_status.get(), isc_tpb_read_committed);
      dpb->insertTag(fb_status.get(), isc_tpb_no_rec_version);
      dpb->insertTag(fb_status.get(), isc_tpb_wait);
      dpb->insertTag(fb_status.get(), isc_tpb_read);
      att = prov->attachDatabase(fb_status.get(), parameters.name.c_str(),
                                 dpb->getBufferLength(fb_status.get()),
                                 dpb->getBuffer(fb_status.get()));
    }
    // вероятно здесь стандартное 'sysdba' и 'masterkey'
    dpb->insertString(fb_status.get(), isc_dpb_user_name,
                      parameters.username.c_str());
    dpb->insertString(fb_status.get(), isc_dpb_password,
                      parameters.password.c_str());
    // TODO: что-то мне не нравится этот кусок
    tra = att->startTransaction(fb_status.get(), 0, NULL);
  } catch (const FbException& e) {
    status_ = STATUS_HAVE_ERROR;
    if (utl != nullptr) {
      char buf[512];
      utl->formatStatus(buf, sizeof(buf), e.getStatus());
      error_.SetError(ERROR_DB_CONNECTION, buf);
    } else {
      error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
    }
  } catch (const std::exception& e) {
    status_ = STATUS_HAVE_ERROR;
    error_.SetError(ERROR_DB_CONNECTION, e.what());
  }
  if (error_.GetErrorCode() != ERROR_SUCCESS_T)
    error_.LogIt();
  return IsAvailable();
}

void DBConnectionFireBird::_firebird_work::ReleaseConnection() {
  if (curs != nullptr)
    curs->release();
  if (stmt != nullptr)
    stmt->release();
  if (tra != nullptr)
    tra->release();
  if (att != nullptr)
    att->release();
  if (dpb != nullptr)
    dpb->dispose();
  if (st != nullptr)
    st->dispose();
  // if (fb_status.get() != nullptr)
  //   fb_status->dispose();
  if (prov != nullptr)
    prov->release();
}

void DBConnectionFireBird::_firebird_work::get_result(metadata_t& result) {
  meta = stmt->getOutputMetadata(fb_status.get());
  builder = meta->getBuilder(fb_status.get());
  unsigned cols = meta->getCount(fb_status.get());
  result.assign(cols, MetaDataField());
  for (unsigned j = 0; j < cols; ++j) {
    unsigned t = meta->getType(fb_status.get(), j);
    if (t == SQL_VARYING || t == SQL_TEXT) {
      builder->setType(fb_status.get(), j, SQL_TEXT);
      result[j].name = meta->getField(fb_status.get(), j);
    }
  }
}

void DBConnectionFireBird::_firebird_work::commit() {
  if (tra != nullptr)
    tra->commit(fb_status.get());
}

void DBConnectionFireBird::_firebird_work::commit_detach() {
  tra->commit(fb_status.get());
  att->detach(fb_status.get());
  tra = nullptr;
}

}  // namespace asp_db
