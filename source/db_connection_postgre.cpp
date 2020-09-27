/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_connection_postgre.h"

#include "db_connection_manager.h"
#include "db_queries_setup.h"
#include "db_query.h"
#include "db_tables.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <numeric>
#include <vector>

#include <assert.h>


namespace asp_db {
#define types_pair(x, y) { x, y }
// #define reverse_types_pair(x, y) { y, x }

/* примеры использования можно посмотреть на github, в репке Jeroen Vermeulen:
     https://github.com/jtv/libpqxx */
namespace postgresql_impl {
/** \brief Мапа соответствий типов данных db_type библиотеки
  *   с PostgreSQL типами данных */
static std::map<db_type, std::string> str_db_types =
    std::map<db_type, std::string> {
  types_pair(db_type::type_empty, ""),
  types_pair(db_type::type_autoinc, "SERIAL"),
  types_pair(db_type::type_uuid, "UUID"),
  types_pair(db_type::type_bool, "BOOL"),
  types_pair(db_type::type_int, "INTEGER"),
  types_pair(db_type::type_long, "BIGINT"),
  types_pair(db_type::type_real, "REAL"),
  types_pair(db_type::type_date, "DATE"),
  types_pair(db_type::type_time, "TIME"),
  // todo: в pgAdmin4 'char' выглядит как 'character' или как 'text'
  types_pair(db_type::type_char_array, "CHAR"),
  types_pair(db_type::type_text, "TEXT"),
};
/** \brief Найти соответствующий строковому представлению
  *   тип данных приложения
  * \todo Получается весьма тонкое место,
  *   из-за специализации времени например */
db_type find_type(const std::string &uppercase_str) {
  for (const auto &x: str_db_types)
    if (x.second == uppercase_str)
      return x.first;
  /* Несколько специфичные типы */
  /* postgres выдаёт строку с широким разворотом по времени,включая
   *   инфо по временному поясу так что это надо расширять */
  std::size_t find_pos = uppercase_str.find("TIME");
  if (find_pos != std::string::npos)
    return db_type::type_time;
  /* про соотношение character/text не знаю что делать */
  return db_type::type_empty;
}

/** \brief Функтор для сетапа полей деревьев условий для PostgreSQL СУБД */
struct where_string_set {
  where_string_set(pqxx::nontransaction *tr)
    : tr(tr) {}

  std::string operator()(db_type t, const std::string &f,
      const std::string &v) {
    if (t == db_type::type_date) {
      return f + " = " + DBConnectionPostgre::DateToPostgreDate(v);
    } else if (t == db_type::type_time) {
      return f + " = " + DBConnectionPostgre::TimeToPostgreTime(v);
    }
    bool need_quote = (t == db_type::type_char_array || t == db_type::type_text);
    // для опции dry_run указатель не проинциализирован
    return (need_quote && tr) ?
        f + " = " + tr->quote(v): f + " = " + v;
  }
public:
  /**
   * \brief Указатель на pqxx транзакцию
   * */
  pqxx::nontransaction *tr;
};

/** \brief Распарсить строку достать из неё все целые числа */
void StringToIntNumbers(const std::string &str, std::vector<int> *result) {
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

/** \brief Получить update|delete действие по символу от БД
  * \note update action code: a = no action, r = restrict,
  *   //   c = cascade, n = set null, d = set default
*/
db_reference_act GetReferenceAct(char postgre_symbol) {
  db_reference_act act = db_reference_act::ref_act_not;
  switch (postgre_symbol) {
    case 'a':
      act = db_reference_act::ref_act_not;
      break;
    case 'n':
      act = db_reference_act::ref_act_set_null;
      break;
    case 'c':
      act = db_reference_act::ref_act_cascade;
      break;
    case 'r':
      act = db_reference_act::ref_act_restrict;
      break;
    default:
      throw DBException(ERROR_DB_REFER_FIELD,
          "Не зарегистрированное действие для внешнего ключа");
  }
  return act;
}
}  // namespace postgresql_impl

DBConnectionPostgre::DBConnectionPostgre(const IDBTables *tables,
    const db_parameters &parameters)
  : DBConnection(tables, parameters) {}

DBConnectionPostgre::DBConnectionPostgre(const DBConnectionPostgre &r)
  : DBConnection(r) {
  pqxx_work.ReleaseConnection();
}

DBConnectionPostgre &DBConnectionPostgre::operator=(
    const DBConnectionPostgre &r) {
  if (&r != this) {
    // копируем
    parameters_ = r.parameters_;
    tables_ = r.tables_;
    // установить дефолтные значения
    error_.Reset();
    status_ = STATUS_DEFAULT;
    is_connected_ = false;
    pqxx_work.ReleaseConnection();
  }
  return *this;
}

DBConnectionPostgre::~DBConnectionPostgre() {
  CloseConnection();
}

std::shared_ptr<DBConnection> DBConnectionPostgre::CloneConnection() {
  return std::shared_ptr<DBConnectionPostgre>(new DBConnectionPostgre(*this));
}

DBConnectionPostgre::db_field_info::db_field_info(const std::string &name,
    db_variable::db_var_type type)
  : name(name), type(type) {}

mstatus_t DBConnectionPostgre::AddSavePoint(const db_save_point &sp) {
  return exec_wrap<db_save_point, void,
     std::stringstream (DBConnectionPostgre::*)(const db_save_point &),
     void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
         sp, nullptr, &DBConnectionPostgre::setupAddSavePointString,
         &DBConnectionPostgre::execAddSavePoint);
}

void DBConnectionPostgre::RollbackToSavePoint(const db_save_point &sp) {
  exec_wrap<db_save_point, void,
     std::stringstream (DBConnectionPostgre::*)(const db_save_point &),
     void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
         sp, nullptr, &DBConnectionPostgre::setupRollbackToSavePoint,
         &DBConnectionPostgre::execRollbackToSavePoint);
}

mstatus_t DBConnectionPostgre::SetupConnection() {
  auto connect_str = setupConnectionString();
  if (!isDryRun()) {
    try {
      pqxx_work.InitConnection(connect_str);
      if (!error_.GetErrorCode()) {
        if (pqxx_work.IsAvailable()) {
          status_ = STATUS_OK;
          is_connected_ = true;
          // отметим начало транзакции
          // todo: вероятно в отдельную функцию вынести
          pqxx_work.GetTransaction()->exec("begin;");
          if (IS_DEBUG_MODE)
            Logging::Append(io_loglvl::debug_logs, "Подключение к БД "
                + parameters_.name);
        } else {
          error_.SetError(ERROR_DB_CONNECTION,
              "Подключение к БД не открыто. Запрос:\n" + connect_str);
          status_ = STATUS_HAVE_ERROR;
        }
      } else {
        error_.SetError(ERROR_DB_CONNECTION,
            "Подключение к БД: инициализация. Запрос:\n" + connect_str);
        status_ = STATUS_HAVE_ERROR;
      }
    } catch(const std::exception &e) {
      error_.SetError(ERROR_DB_CONNECTION,
          "Подключение к БД: exception. Запрос:\n" + connect_str
          + "\nexception what: " + e.what());
      status_ = STATUS_HAVE_ERROR;
    }
  } else {
    status_ = STATUS_OK;
    Logging::Append(io_loglvl::debug_logs, "dry_run connect:" + connect_str);
    Logging::Append(io_loglvl::debug_logs, "dry_run transaction begin");
  }
  return status_;
}

void DBConnectionPostgre::CloseConnection() {
  if (pqxx_work.pconnect_) {
    // если собирали транзакцию - закрыть
    if (pqxx_work.IsAvailable())
      pqxx_work.GetTransaction()->exec("commit;");
    pqxx_work.pconnect_->disconnect();
    pqxx_work.ReleaseConnection();
    is_connected_ = false;
    error_.Reset();
    if (IS_DEBUG_MODE)
      Logging::Append(io_loglvl::debug_logs, "Закрытие соединения c БД "
          + parameters_.name);
  }
  if (isDryRun()) {
    status_ = STATUS_OK;
    Logging::Append(io_loglvl::debug_logs, "dry_run commit and disconect");
  }
}

mstatus_t DBConnectionPostgre::IsTableExists(db_table t, bool *is_exists) {
  return exec_wrap<db_table, bool,
     std::stringstream (DBConnectionPostgre::*)(db_table),
     void (DBConnectionPostgre::*)(const std::stringstream &, bool *)>(
         t, is_exists, &DBConnectionPostgre::setupTableExistsString,
         &DBConnectionPostgre::execIsTableExists);
}

// todo: в методе смешаны уровни абстракции,
//   разбить на парочку/другую методов или функций
mstatus_t DBConnectionPostgre::GetTableFormat(db_table t,
    db_table_create_setup *fields) {
  assert(0 && "не оттестированно");
  // такс, собираем
  //   колонки
  std::vector<db_field_info> exists_cols;
  mstatus_t res = exec_wrap<db_table, std::vector<db_field_info>,
      std::stringstream (DBConnectionPostgre::*)(db_table),
      void (DBConnectionPostgre::*)(const std::stringstream &, std::vector<db_field_info> *)>(
          t, &exists_cols, &DBConnectionPostgre::setupGetColumnsInfoString,
          &DBConnectionPostgre::execGetColumnInfo);
  if (is_status_ok(res)) {
    fields->table = t;
    auto table_name = tables_->GetTableName(t);
    auto ptr_table_fields = tables_->GetFieldsCollection(t);
    std::map<std::string, db_variable_id> fields_names_map;
    for (const auto &x: *ptr_table_fields)
      fields_names_map.emplace(x.fname, x.fid);
    fields->fields.clear();
    for (const auto &x: exists_cols) {
      auto id_it = fields_names_map.find(x.name);
      if (id_it != fields_names_map.end()) {
        fields->fields.push_back(db_variable(id_it->second, x.name.c_str(),
            x.type, db_variable::db_variable_flags()));
      } else {
        // поле с таким именем даже не нашлось, вставим заглушку
        fields->fields.push_back(db_variable(UNDEFINED_COLUMN, x.name.c_str(),
            db_variable::db_var_type::type_empty, db_variable::db_variable_flags()));
      }
    }
  }
  //   закончили про колонку

  // update|delete методы внешнего ключа
  struct fk_UpDel {
    db_reference_act up;
    db_reference_act del;
  };
  std::map<std::string, fk_UpDel> fk_map;

  //   ограничения начинаем
  pqxx::result constrains;
    res = exec_wrap<db_table, pqxx::result,
      std::stringstream (DBConnectionPostgre::*)(db_table),
      void (DBConnectionPostgre::*)(const std::stringstream &, pqxx::result *)>(
          t, &constrains, &DBConnectionPostgre::setupGetConstrainsString,
          &DBConnectionPostgre::execGetConstrainsString);
  // обход по ограничениям - первичным и внешним ключам, уникальным комплексам
  merror_t error;
  if (is_status_ok(res)) {
    for (pqxx::const_result_iterator::reference row: constrains) {
      // массив индексов
      std::vector<int> indexes;
      auto conkey = row["conkey"];
      if (conkey != row.end()) {
        indexes.clear();
        std::string indexes_str = conkey.as<std::string>();
        postgresql_impl::StringToIntNumbers(indexes_str, &indexes);
        if (indexes.empty()) {
          error = error_.SetError(ERROR_STR_PARSE_ST,
              "Ошибка парсинга строки: '" + indexes_str + "' - пустой результат");
          break;
        }
        // такс, здесь храним имена колонок, которые достанем из ряда row
        auto ct = row["contype"];
        if (ct != row.end()) {
          char cs = '!';
          std::string tmp = ct.as<std::string>();
          if (!tmp.empty())
            cs = tmp[0];
          switch (cs) {
            // primary key
            case 'p':
              setConstrainVector(indexes, fields->fields, &fields->pk_string.fnames);
              break;
            // unique
            case 'u':
              fields->unique_constrains.push_back(
                  db_table_create_setup::unique_constrain());
              setConstrainVector(indexes, fields->fields,
                  &fields->unique_constrains.back());
              fields->unique_constrains.clear();
              break;
            // foreign key(separate function)
            case 'f':
              {
                // Foreign key update action code: a = no action, r = restrict,
                //   c = cascade, n = set null, d = set default
                // нужно достать имя поля, оно одно, но функция под вектор
                std::vector<std::string> name;
                setConstrainVector(indexes, fields->fields, &name);
                if (!name.empty()) {
                  try {
                    fk_UpDel ud;
                    auto c = row["confupdtype"];
                    if (c != row.end()) {
                      tmp = c.as<std::string>();
                      ud.up = postgresql_impl::GetReferenceAct(
                          tmp.empty() ? '!': tmp[0]);
                      c = row["confdeltype"];
                      if (c != row.end()) {
                        tmp = c.as<std::string>();
                        ud.del = postgresql_impl::GetReferenceAct(
                            tmp.empty() ? '!': tmp[0]);
                      } else {
                        error = ERROR_GENERAL_T;
                        Logging::Append("foreign key constrain error: act delete");
                      }
                    } else {
                      Logging::Append("foreign key constrain error: act update");
                      error = ERROR_GENERAL_T;
                    }
                    fk_map.emplace(trim_str(name[0]), ud);
                  } catch (DBException &e) {
                    e.LogException();
                    error = e.GetError();
                  }
                }
              }
              break;
            // exclusion constraint
            case 'x':
            // check constraint
            case 'c':
            // constraint trigger
            case 't':
            default:
              break;
          }
        }
      }
    }
  } else {
    if (!(error = error_.GetErrorCode()))
      error = ERROR_DB_OPERATION;
  }
  //   ограничения закончили

  //   внешние ключи
  pqxx::result fkeys;
  if (!error)
    res = exec_wrap<db_table, pqxx::result,
      std::stringstream (DBConnectionPostgre::*)(db_table),
      void (DBConnectionPostgre::*)(const std::stringstream &, pqxx::result *)>(
          t, &fkeys, &DBConnectionPostgre::setupGetForeignKeys,
          &DBConnectionPostgre::execGetForeignKeys);
  if (!error && is_status_ok(res)) {
    // foreign_column_name, foreign_table_name
    for (pqxx::const_result_iterator::reference row: fkeys) {
      bool success = false;
      // таблица со ссылкой
      auto row_it = row["column_name"];
      std::string col_name = "";
      if (row_it != row.end()) {
        col_name = row_it.as<std::string>();
        if (row_it != row.end()) {
          row_it = row["foreign_table_name"];
          if (row_it != row.end()) {
            std::string f_table = row_it.as<std::string>();
            row_it = row["foreign_column_name"];
            if (row_it != row.end()) {
              std::string f_col = row_it.as<std::string>();
              auto du = fk_map.find(col_name);
              if (du != fk_map.end()) {
                fields->ref_strings->push_back(
                    db_reference(col_name, tables_->StrToTableCode(f_table),
                    f_col, true, du->second.up, du->second.del));
                success = true;
              } else {
                error = ERROR_DB_REFER_FIELD;
              }
            }
          }
        }
      }
      if (!success) {
        error = error_.SetError(ERROR_DB_REFER_FIELD,
            "Ошибка составления строки внешнего ключа");
        break;
      }
    }
  }
  if (error_.GetErrorCode())
    res = STATUS_HAVE_ERROR;
  return res;
}

mstatus_t DBConnectionPostgre::CheckTableFormat(
    const db_table_create_setup &fields) {
  /* todo */
  assert(0);
  return 0;
}

mstatus_t DBConnectionPostgre::UpdateTable(
    const db_table_create_setup &fields) {
  /* todo */
  assert(0);
  return STATUS_HAVE_ERROR;
}

mstatus_t DBConnectionPostgre::CreateTable(
    const db_table_create_setup &fields) {
  return exec_wrap<db_table_create_setup, void,
    std::stringstream (DBConnectionPostgre::*)(const db_table_create_setup &),
    void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
        fields, nullptr, &DBConnectionPostgre::setupCreateTableString,
        &DBConnectionPostgre::execCreateTable);
}

mstatus_t DBConnectionPostgre::DropTable(const db_table_drop_setup &drop) {
  return exec_wrap<db_table_drop_setup, void,
     std::stringstream (DBConnectionPostgre::*)(const db_table_drop_setup &),
     void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
         drop, nullptr, &DBConnectionPostgre::setupDropTableString,
         &DBConnectionPostgre::execDropTable);
}

mstatus_t DBConnectionPostgre::InsertRows(
    const db_query_insert_setup &insert_data, id_container *id_vec) {
  pqxx::result result;
  mstatus_t status = exec_wrap<db_query_insert_setup, pqxx::result,
      std::stringstream (DBConnectionPostgre::*)(const db_query_insert_setup &),
      void (DBConnectionPostgre::*)(const std::stringstream &, pqxx::result *)>(
          insert_data, &result, &DBConnectionPostgre::setupInsertString,
          &DBConnectionPostgre::execInsert);
  if (id_vec)
    for (pqxx::const_result_iterator::reference row: result)
      id_vec->id_vec.push_back(row[0].as<int>());
  return status;
}

mstatus_t DBConnectionPostgre::DeleteRows(
    const db_query_delete_setup &delete_data) {
  return exec_wrap<db_query_delete_setup, void,
     std::stringstream (DBConnectionPostgre::*)(const db_query_delete_setup &),
     void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
         delete_data, nullptr, &DBConnectionPostgre::setupDeleteString,
         &DBConnectionPostgre::execDelete);
}

mstatus_t DBConnectionPostgre::SelectRows(
    const db_query_select_setup &select_data,
    db_query_select_result *result_data) {
  pqxx::result result;
  result_data->values_vec.clear();
  mstatus_t res = exec_wrap<db_query_select_setup, pqxx::result,
     std::stringstream (DBConnectionPostgre::*)(const db_query_select_setup &),
     void (DBConnectionPostgre::*)(const std::stringstream &, pqxx::result *)>(
         select_data, &result, &DBConnectionPostgre::setupSelectString,
         &DBConnectionPostgre::execSelect);
  for (pqxx::const_result_iterator::reference row: result) {
    db_query_basesetup::row_values rval;
    db_query_basesetup::field_index ind = 0;
    for (const auto &field: select_data.fields) {
      std::string fieldname = field.fname;
      if (row[fieldname] != row.end())
        rval.emplace(ind, row[fieldname].c_str());
      ++ind;
    }
    result_data->values_vec.push_back(rval);
  }
  return res;
}

mstatus_t DBConnectionPostgre::UpdateRows(
    const db_query_update_setup &update_data) {
  return exec_wrap<db_query_update_setup, void,
     std::stringstream (DBConnectionPostgre::*)(const db_query_update_setup &),
     void (DBConnectionPostgre::*)(const std::stringstream &, void *)>(
         update_data, nullptr, &DBConnectionPostgre::setupUpdateString,
         &DBConnectionPostgre::execUpdate);
}

std::string DBConnectionPostgre::setupConnectionString() {
  std::stringstream connect_ss;
  connect_ss << "dbname = " << parameters_.name << " ";
  connect_ss << "user = " << parameters_.username << " ";
  connect_ss << "password = " << parameters_.password << " ";
  connect_ss << "hostaddr = " << parameters_.host << " ";
  connect_ss << "port = " << parameters_.port;
  return connect_ss.str();
}

std::stringstream DBConnectionPostgre::setupTableExistsString(db_table t) {
  std::stringstream select_ss;
  select_ss << "SELECT EXISTS ( SELECT 1 FROM information_schema.tables "
      "WHERE table_schema = 'public' AND table_name = '" <<
      tables_->GetTableName(t) << "');";
  return select_ss;
}
std::stringstream DBConnectionPostgre::setupGetColumnsInfoString(db_table t) {
  std::stringstream select_ss;
  select_ss << "SELECT column_name, data_type FROM INFORMATION_SCHEMA.COLUMNS "
      "WHERE TABLE_NAME = '" << tables_->GetTableName(t) << "';";
  return select_ss;
}
std::stringstream DBConnectionPostgre::setupGetConstrainsString(db_table t) {
  std::stringstream sstr;
  sstr << "SELECT con.* " <<
       "FROM pg_catalog.pg_constraint con " <<
            "INNER JOIN pg_catalog.pg_class rel " <<
                       "ON rel.oid = con.conrelid " <<
            "INNER JOIN pg_catalog.pg_namespace nsp " <<
                       "ON nsp.oid = connamespace ";
  sstr << "WHERE nsp.nspname = 'public' AND rel.relname = ";
  sstr << "'" << tables_->GetTableName(t) << "';";
  return sstr;
}

std::stringstream DBConnectionPostgre::setupGetForeignKeys(db_table t) {
  std::stringstream sstr;
  sstr << "SELECT " <<
        "tc.table_schema, " <<
        "tc.constraint_name, " <<
        "tc.table_name, " <<
        "kcu.column_name, " <<
        "ccu.table_schema AS foreign_table_schema, " <<
        "ccu.table_name AS foreign_table_name, " <<
        "ccu.column_name AS foreign_column_name " <<
    "FROM " <<
        "information_schema.table_constraints AS tc " <<
    "JOIN information_schema.key_column_usage AS kcu " <<
        "ON tc.constraint_name = kcu.constraint_name " <<
        "AND tc.table_schema = kcu.table_schema " <<
    "JOIN information_schema.constraint_column_usage AS ccu " <<
        "ON ccu.constraint_name = tc.constraint_name " <<
        "AND ccu.table_schema = tc.table_schema ";
  sstr << "WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = ";
  sstr << "'" << tables_->GetTableName(t) << "';";
  return sstr;
}

std::stringstream DBConnectionPostgre::setupInsertString(
    const db_query_insert_setup &fields) {
  if (fields.values_vec.empty()) {
    error_.SetError(ERROR_DB_VARIABLE, "Нет данных для INSERT операции");
    return std::stringstream();
  }
  if (fields.values_vec[0].empty()) {
    error_.SetError(ERROR_DB_VARIABLE, "INSERT операция для пустых строк");
    return std::stringstream();
  }
  std::string fnames = "INSERT INTO " + tables_->GetTableName(fields.table) + " (";
  std::vector<std::string> vals;
  std::vector<std::string> rows(fields.values_vec.size());
  // set fields
  auto &row = fields.values_vec[0];
  for (auto &x: row)
    fnames += std::string(fields.fields[x.first].fname) + ", ";
  fnames.replace(fnames.size() - 2, fnames.size() - 1, ")");

  for (const auto &row: fields.values_vec) {
    std::string value = " (";
    for (const auto &x: row) {
      if (x.first >= 0 && (x.first < fields.fields.size())) {
        addVariableToString(&value, fields.fields[x.first], x.second);
      } else {
        Logging::Append(io_loglvl::debug_logs, "Ошибка индекса операции INSERT.\n"
            "\tДля таблицы " + tables_->GetTableName(fields.table));
      }
    }
    value.replace(value.size() - 2, value.size(), "),");
    vals.emplace_back(value);
  }
  vals.back().replace(vals.back().size() - 1, vals.back().size(), " ");
  std::stringstream sstr;
  sstr << fnames << " VALUES ";
  for (const auto &x: vals)
    sstr << x;
  sstr << "RETURNING " << tables_->GetIdColumnName(fields.table) << ";";
  return sstr;
}

std::stringstream DBConnectionPostgre::setupDeleteString(
    const db_query_delete_setup &fields) {
  std::stringstream sstr;
  postgresql_impl::where_string_set ws(pqxx_work.GetTransaction());
  sstr << "DELETE FROM " << tables_->GetTableName(fields.table);
  std::string str = "";
  if (fields.where_condition != nullptr)
    str = fields.where_condition->GetString(ws);
  if (!trim_str(str).empty())
    sstr << " WHERE " << str;
  sstr << ";";
  return sstr;
}
std::stringstream DBConnectionPostgre::setupSelectString(
    const db_query_select_setup &fields) {
  std::stringstream sstr;
  postgresql_impl::where_string_set ws(pqxx_work.GetTransaction());
  sstr << "SELECT * FROM " << tables_->GetTableName(fields.table);
  std::string str = "";
  if (fields.where_condition != nullptr)
    str = fields.where_condition->GetString(ws);
  if (!trim_str(str).empty())
    sstr << " WHERE " << str;
  sstr << ";";
  return sstr;
}
std::stringstream DBConnectionPostgre::setupUpdateString(
    const db_query_update_setup &fields) {
  postgresql_impl::where_string_set ws(pqxx_work.GetTransaction());
  std::stringstream sstr;
  if (!fields.values.empty()) {
    sstr << "UPDATE " << tables_->GetTableName(fields.table)
        << " SET ";
    std::string set_str = "";
    for (const auto &x: fields.values)
      set_str += std::string(fields.fields[x.first].fname)
          + " = " + x.second + ",";
    set_str[set_str.size() - 1] = ' ';
    sstr << set_str;
    std::string str = "";
    if (fields.where_condition != nullptr)
      str = fields.where_condition->GetString(ws);
    if (!trim_str(str).empty())
      sstr << " WHERE " << fields.where_condition->GetString(ws);
    sstr << ";";
  }
  return sstr;
}

void DBConnectionPostgre::execWithoutReturn(const std::stringstream &sstr) {
  auto tr = pqxx_work.GetTransaction();
  if (tr)
    tr->exec0(sstr.str());
}
void DBConnectionPostgre::execWithReturn(const std::stringstream &sstr,
    pqxx::result *result) {
  auto tr = pqxx_work.GetTransaction();
  if (tr)
    *result = tr->exec(sstr.str());
}

void DBConnectionPostgre::execAddSavePoint(
    const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execRollbackToSavePoint(
    const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execIsTableExists(
    const std::stringstream &sstr, bool *is_exists) {
  auto tr = pqxx_work.GetTransaction();
  if (tr) {
    pqxx::result trres(tr->exec(sstr.str()));
    if (!trres.empty()) {
      std::string ex = trres.begin()[0].as<std::string>();
      *is_exists = (ex == "t") ? true : false;
      if (IS_DEBUG_MODE)
       Logging::Append(io_loglvl::debug_logs, "Ответ на запрос БД:"
            + sstr.str() + "\t'" + trres.begin()[0].as<std::string>() + "'\n") ;
    }
  }
}
/* todo: так-с, нужно вынести названия служебных столбцов/полей
 *   'column_name' и 'data_type' в енумчик или дефайн */
void DBConnectionPostgre::execGetColumnInfo(
    const std::stringstream &sstr, std::vector<db_field_info> *columns_info) {
  columns_info->clear();
  auto tr = pqxx_work.GetTransaction();
  if (tr) {
    pqxx::result trres(tr->exec(sstr.str()));
    if (!trres.empty()) {
      for (const auto &x: trres) {
        auto row = x["column_name"];
        if (row != x.end()) {
          std::string name = trim_str(row.as<std::string>());
          row = x["data_type"];
          if (row != x.end()) {
            std::string type_str = trim_str(row.as<std::string>());
            std::transform(type_str.begin(), type_str.end(),
                type_str.begin(), toupper);
            columns_info->push_back(
                db_field_info(name, postgresql_impl::find_type(type_str)));
          }
        }
      }
      if (IS_DEBUG_MODE) {
        std::string cols = std::accumulate(
            columns_info->begin(), columns_info->end(), std::string(),
            [](std::string &r, const db_field_info &n) { return r + n.name; });
        Logging::Append(io_loglvl::debug_logs, "Ответ на запрос БД:"
            + sstr.str() + "\t'" + cols + "'\n") ;
      }
    }
  }
}
void DBConnectionPostgre::execGetConstrainsString(const std::stringstream &sstr,
    pqxx::result *result) {
  execWithReturn(sstr, result);
}
void DBConnectionPostgre::execGetForeignKeys(const std::stringstream &sstr,
    pqxx::result *result) {
  execWithReturn(sstr, result);
}
void DBConnectionPostgre::execAddColumn(const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execCreateTable(const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execDropTable(const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execInsert(const std::stringstream &sstr,
    pqxx::result *result) {
  execWithReturn(sstr, result);
}
void DBConnectionPostgre::execDelete(const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}
void DBConnectionPostgre::execSelect(const std::stringstream &sstr,
    pqxx::result *result) {
  execWithReturn(sstr, result);
}
void DBConnectionPostgre::execUpdate(const std::stringstream &sstr, void *) {
  execWithoutReturn(sstr);
}

merror_t DBConnectionPostgre::setConstrainVector(const std::vector<int> &indexes,
    const db_fields_collection &fields, std::vector<std::string> *output) {
  merror_t error = ERROR_SUCCESS_T;
  output->clear();
  for (const size_t i: indexes) {
    if (i < fields.size()) {
      output->push_back(fields[i].fname);
    } else {
      error = error_.SetError(ERROR_DB_TABLE_PKEY,
          "Ошибка создания первичного ключа по параметрам из БД: "
          "индекс столбца вне границ таблицы");
    }
  }
  return error;
}

void DBConnectionPostgre::addVariableToString(std::string *str_p,
    const db_variable &var, const std::string &value) {
  db_variable::db_var_type t = var.type;
  if (var.flags.is_array && t != db_type::type_char_array) {
    std::string str = "ARRAY[";
    std::vector<std::string> vec;
    if (is_status_ok(db_variable::TranslateToVector(value, &vec))) {
      for (const auto &x: vec)
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

std::string DBConnectionPostgre::getVariableValue(const db_variable &var,
    const std::string &value) {
  db_variable::db_var_type t = var.type;
  std::string str = "";
  if ((t == db_type::type_char_array ||
      t == db_type::type_text)) {
    auto txn = pqxx_work.GetTransaction();
    if (txn)
      str = txn->quote(value) + ", ";
  } else if (t == db_type::type_date) {
    str = DateToPostgreDate(value) + ", ";
  } else if (t == db_type::type_time) {
    str = TimeToPostgreTime(value) + ", ";
  } else {
    str += value + ", ";
  }
  return str;
}

std::string DBConnectionPostgre::db_variable_to_string(
    const db_variable &dv) {
  std::stringstream ss;
  merror_t ew = dv.CheckYourself();
  if (!ew) {
    ss << dv.fname << " ";
    auto itDBtype = postgresql_impl::str_db_types.find(dv.type);
    if (itDBtype != postgresql_impl::str_db_types.end()) {
      // todo: вроде как здесь пример для массива символов
      //   в обычном массиве скобки квадратны, и пусты могут быть
      // алсо, как насчёт массива массивов
      ss << postgresql_impl::str_db_types[dv.type];
      if (dv.flags.is_array) {
        // если dv.len == 0, то массив не ограничен(так можно)
        std::string len_str = (dv.len > 0) ? std::to_string(dv.len) : "";
        if (dv.type == db_type::type_char_array)
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
  } else {
    error_.SetError(ew,
        "Проверка параметров поля таблицы завершилось ошибкой");
  }
  return ss.str();
}

std::string DBConnectionPostgre::DateToPostgreDate(const std::string &date) {
  // postgres need
  char s[16] = {0};
  int i = 0;
  for_each (date.begin(), date.end(),
      [&s, &i](char c) { s[i++] = (c == '/') ? '-' : c; });
  return "'" + std::string(s) + "'";
}

std::string DBConnectionPostgre::TimeToPostgreTime(const std::string &time) {
  return "'" + time + "'";
}

std::string DBConnectionPostgre::PostgreDateToDate(const std::string &pdate) {
  char s[16] = {0};
  int i = 0;
  for_each (pdate.begin(), pdate.end(),
      [&s, &i](char c) { s[i++] = (c == '-') ? '/' : c; });
  return s;
}

std::string DBConnectionPostgre::PostgreTimeToTime(const std::string &ptime) {
  return ptime;
}
}  // namespace asp_db
