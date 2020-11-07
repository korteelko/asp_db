/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_CONNECTION_POSTGRESQL_H_
#define _DATABASE__DB_CONNECTION_POSTGRESQL_H_

#include "Common.h"
#include "ErrorWrap.h"
#include "Logging.h"
#include "db_connection.h"

#include <pqxx/pqxx>

#include <functional>
#include <memory>
#include <sstream>
#include <string>

#define POSTGRE_DRYRUN_LOGGER "postgre_logger"
#define POSTGRE_DRYRUN_LOGFILE "postgre_logs"

namespace asp_db {
// смотри страницу:
// https://www.tutorialspoint.com/postgresql/postgresql_c_cpp.htm
/**
 * \brief Реализация DBConnection для postgresql
 * */
class DBConnectionPostgre final : public DBConnection {
  ADD_TEST_CLASS(DBConnectionPostgreProxy)
  /**
   * \brief Данные полей таблицы, которые можно из неё вытащить
   *
   * \warning Это вообще используется?
   * */
  struct db_field_info {
   public:
    /** \brief Имя поля */
    std::string name;
    /** \brief Тип данных */
    db_variable_type type;
    // int len;
   public:
    db_field_info(const std::string& name, db_variable_type type);
  };

 public:
  DBConnectionPostgre(const IDBTables* tables,
                      const db_parameters& parameters,
                      PrivateLogging* logger = nullptr);

  ~DBConnectionPostgre() override;

  std::shared_ptr<DBConnection> CloneConnection() override;

  mstatus_t AddSavePoint(const db_save_point& sp) override;
  void RollbackToSavePoint(const db_save_point& sp) override;

  mstatus_t SetupConnection() override;
  void CloseConnection() override;

  mstatus_t IsTableExists(db_table t, bool* is_exists) override;
  mstatus_t GetTableFormat(db_table t, db_table_create_setup* fields) override;
  mstatus_t CheckTableFormat(const db_table_create_setup& fields) override;
  mstatus_t UpdateTable(const db_table_create_setup& fields) override;
  mstatus_t CreateTable(const db_table_create_setup& fields) override;
  mstatus_t DropTable(const db_table_drop_setup& drop) override;

  mstatus_t InsertRows(const db_query_insert_setup& insert_data,
                       id_container* id_vec) override;
  mstatus_t DeleteRows(const db_query_delete_setup& delete_data) override;
  mstatus_t SelectRows(const db_query_select_setup& select_data,
                       db_query_select_result* result_data) override;
  mstatus_t UpdateRows(const db_query_update_setup& update_data) override;

  /**
   * \brief Переформатировать строку общего формата даты
   *   "'yyyy/mm/dd'" к формату используемому БД("yyyy-mm-dd")
   * */
  static std::string DateToPostgreDate(const std::string& date);
  /**
   * \brief Переформатировать строку общего формата времени
   *   'hh:mm' к формату используемому БД
   * */
  static std::string TimeToPostgreTime(const std::string& time);
  /**
   * \brief Переформатировать строку общего формата даты
   *   БД("yyyy-mm-dd") к формату "'yyyy/mm/dd'"
   * */
  static std::string PostgreDateToDate(const std::string& pdate);
  /**
   * \brief Переформатировать строку общего формата времени БД
   *   к формату 'hh:mm'
   * */
  static std::string PostgreTimeToTime(const std::string& ptime);

 private:
  DBConnectionPostgre(const DBConnectionPostgre& r);
  DBConnectionPostgre& operator=(const DBConnectionPostgre& r);

  /** \brief Добавить бэкап точку перед операцией изменяющей
   *   состояние таблицы */
  mstatus_t addSavePoint();

  /** \brief Шаблон функции оборачивающий операции с БД:
   *  1) Собрать запрос.
   *  2) Отправить и обработать результат.
   *  3) Обработать ошибки.
   * \param data данные для сборки запроса
   * \param res указатель на хранилище результата
   * \param setup_m функция сборки текста запроса
   * \param exec_m функция отправки запроса, парсинга результата */
  template <class DataT, class OutT, class SetupF, class ExecF>
  mstatus_t exec_wrap(const DataT& data,
                      OutT* res,
                      SetupF setup_m,
                      ExecF exec_m) {
    // setup content of query(call some function 'setup*String' from
    //   list of function below)
    std::stringstream sstr = std::invoke(setup_m, *this, data);
    sstr.seekg(0, std::ios::end);
    auto sstr_len = sstr.tellg();
    if (pqxx_work.IsAvailable() && sstr_len) {
      try {
        // execute query
        std::invoke(exec_m, *this, sstr, res);
        if (IS_DEBUG_MODE)
          Logging::Append(io_loglvl::debug_logs,
                          "Debug mode: \n\tЗапрос БД: " + sstr.str() + "\n\t");
      } catch (const pqxx::undefined_table& e) {
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_TABLE_EXISTS,
                        "Exception text: " + std::string(e.what()) +
                            "\n Query: " + e.query());
      } catch (const pqxx::integrity_constraint_violation& e) {
        // исключение пробрасывается при ошибке создания строки
        //   например если строка с таким комплексом уникальных
        //   значений уже существует
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_SQL_QUERY,
                        "Exception text: " + std::string(e.what()) +
                            "\n Query: " + e.query());
      } catch (const std::exception& e) {
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION,
                        "Подключение к БД: exception. Запрос:\n" + sstr.str() +
                            "\nexception what: " + e.what());
      }
    } else {
      if (isDryRun()) {
        status_ = STATUS_OK;
        // dry_run_ programm setup
        if (sstr_len) {
          passToLogger(io_loglvl::info_logs, POSTGRE_DRYRUN_LOGGER,
                       "dry_run: " + sstr.str());
        } else {
          passToLogger(io_loglvl::info_logs, POSTGRE_DRYRUN_LOGGER,
                       "dry_run: 'empty query!'");
        }
      } else {
        // error - not connected
        error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
        status_ = STATUS_NOT;
      }
    }
    return status_;
  }

  /* функции собирающие строку запроса */
  /** \brief Собрать строку подключения к БД */
  std::string setupConnectionString();
  std::stringstream setupTableExistsString(db_table t) override;
  /** \brief Собрать строку получения информации о столбцах */
  std::stringstream setupGetColumnsInfoString(db_table t) override;
  /** \brief Собрать строку получения ограничений таблицы */
  std::stringstream setupGetConstrainsString(db_table t);
  /** \brief Собрать строку получения внешних ключей */
  std::stringstream setupGetForeignKeys(db_table t);
  std::stringstream setupInsertString(
      const db_query_insert_setup& fields) override;
  std::stringstream setupDeleteString(
      const db_query_delete_setup& fields) override;
  std::stringstream setupSelectString(
      const db_query_select_setup& fields) override;
  std::stringstream setupUpdateString(
      const db_query_update_setup& fields) override;

  std::string db_variable_to_string(const db_variable& dv) override;

  /* функции исполнения запросов */
  /** \brief Обычный запрос к БД без возвращаемого результата */
  void execWithoutReturn(const std::stringstream& sstr);
  /** \brief Запрос к БД с получением результата */
  void execWithReturn(const std::stringstream& sstr, pqxx::result* result);

  /** \brief Запрос создания метки сохранения */
  void execAddSavePoint(const std::stringstream& sstr, void*);
  /** \brief Запрос отката к метке сохранения */
  void execRollbackToSavePoint(const std::stringstream& sstr, void*);
  /** \brief Запрос существования таблицы */
  void execIsTableExists(const std::stringstream& sstr, bool* is_exists);
  /** \brief Запрос существования таблицы */
  void execGetColumnInfo(const std::stringstream& sstr,
                         std::vector<db_field_info>* columns_info);
  /** \brief Запрос получения ограничений таблицы */
  void execGetConstrainsString(const std::stringstream& sstr,
                               pqxx::result* result);
  /** \brief Запрос получения внешних ключей таблицы */
  void execGetForeignKeys(const std::stringstream& sstr, pqxx::result* result);
  /** \brief Запрос добавления колонки в таблицу */
  void execAddColumn(const std::stringstream& sstr, void*);
  /** \brief Запрос создания таблицы */
  void execCreateTable(const std::stringstream& sstr, void*);
  /** \brief Запрос удаления таблицы */
  void execDropTable(const std::stringstream& sstr, void*);

  /** \brief Запрос на добавление строки */
  void execInsert(const std::stringstream& sstr, pqxx::result* result);
  /** \brief Запрос на удаление строки */
  void execDelete(const std::stringstream& sstr, void*);
  /** \brief Запрос выборки из таблицы
   * \note изменить 'void *' выход на 'pqxx::result *result' */
  void execSelect(const std::stringstream& sstr, pqxx::result* result);
  /** \brief Запрос на обновление строки */
  void execUpdate(const std::stringstream& sstr, void*);

  /** \brief Собрать вектор имён ограничений
   * \param indexes Индексы полей из fields
   * \param fields Контейнер полей
   * \param output Указатель на вектор выходных данных,
   *   именно в него записываются по индексам их indexes
   *   имена полей из fields */
  merror_t setConstrainVector(const std::vector<int>& indexes,
                              const db_fields_collection& fields,
                              std::vector<std::string>* output);

  /** \brief Добавить к строке 'str_p' строку, собранную по данным
   *   'var' и 'value'
   * \param str_p Указатель на строку для добавления
   * \param var Параметры добавляемого значения
   * \param value Строковое представление параметра */
  void addVariableToString(std::string* str_p,
                           const db_variable& var,
                           const std::string& value);
  /**
   * \brief Получить строковое представление переменной
   * \param var Параметры добавляемого значения
   * \param value Строковое представление параметра
   * */
  std::string getVariableValue(const db_variable& var,
                               const std::string& value);

  /**
   * \brief Получить подстроку INSERT запроса соответствующую отработке
   *   `act` для существующих данных
   * \param act Действия над данными таблиц уже существующими в БД
   *
   * Функция для INSERT запроса вернёт подстроку описывающую реакцию на
   * наличие конфликтов включаемых данных(из db_query_insert_setup) и уже
   * существующих в БД.
   * */
  std::string getOnExistActForInsert(insert_on_exists_act act);

 private:
  /**
   * \brief Аноннимная обёртка над параметрами работы с Postgre:
   *   поключения, транзакции
   * */
  struct {
   public:
    /**
     * \brief Инициализировать параметры соединения, транзакции
     * */
    bool InitConnection(const std::string& connect_str) {
      pconnect_ =
          std::unique_ptr<pqxx::connection>(new pqxx::connection(connect_str));
      if (pconnect_)
        if (pconnect_->is_open())
          work_ = std::unique_ptr<pqxx::nontransaction>(
              new pqxx::nontransaction(*pconnect_));
      return IsAvailable();
    }
    /**
     * \brief Освободить параметры подключения
     * */
    void ReleaseConnection() {
      work_ = nullptr;
      pconnect_ = nullptr;
    }
    /**
     * \brief Проверить установки текущей транзаккции
     * */
    bool IsAvailable() const { return work_ != nullptr; }
    /**
     * \brief Указатель на транзакцию
     * */
    pqxx::nontransaction* GetTransaction() const { return work_.get(); }

   public:
    /**
     * \brief Указатель на объект подключения
     * */
    std::unique_ptr<pqxx::connection> pconnect_ = nullptr;
    /**
     * \brief Указатель на транзакцию
     * */
    std::unique_ptr<pqxx::nontransaction> work_ = nullptr;

  } pqxx_work;
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_CONNECTION_POSTGRESQL_H_
