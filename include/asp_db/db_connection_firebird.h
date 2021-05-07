/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_CONNECTION_FIREBIRD_H_
#define _DATABASE__DB_CONNECTION_FIREBIRD_H_

#include "asp_db/db_connection.h"

#include "asp_utils/Common.h"
#include "asp_utils/ErrorWrap.h"
#include "asp_utils/Logging.h"

#include <firebird/Interface.h>

#include <functional>
#include <memory>
#include <sstream>
#include <string>

#define FIREBIRD_DRYRUN_LOGGER "firebird_logger"
#define FIREBIRD_DRYRUN_LOGFILE "firebird_logs"

#define SAMPLES_DIALECT SQL_DIALECT_V6

namespace asp_db {
using namespace Firebird;
/**
 * \brief Концепт строкового типа SQL запроса
 * */
template <typename T>
concept SQLType = std::is_same<T, std::string>::value || std::
    is_same<T, std::stringstream>::value
    // hmmmmmmmmm???
    || std::is_same<T, const char*>::value;

/**
 * \brief Кэш метаданных полей
 * */
struct MetaDataField {
  const char* name;
  unsigned length, offset;
};
typedef std::vector<MetaDataField> metadata_t;

/**
 * \brief Реализация DBConnection для firebird
 * */
class DBConnectionFireBird final : public DBConnection {
 public:
  DBConnectionFireBird(const IDBTables* tables,
                       const db_parameters& parameters,
                       PrivateLogging* logger = nullptr);
  ~DBConnectionFireBird() override;

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
  static std::string DateToFireBirdDate(const std::string& date);
  /**
   * \brief Переформатировать строку общего формата времени
   *   'hh:mm' к формату используемому БД
   * */
  static std::string TimeToFireBirdTime(const std::string& time);
  /**
   * \brief Переформатировать строку общего формата даты
   *   БД("yyyy-mm-dd") к формату "'yyyy/mm/dd'"
   * */
  static std::string FireBirdDateToDate(const std::string& pdate);
  /**
   * \brief Переформатировать строку общего формата времени БД
   *   к формату 'hh:mm'
   * */
  static std::string FireBirdTimeToTime(const std::string& ptime);

 private:
  DBConnectionFireBird(const DBConnectionFireBird& r);
  DBConnectionFireBird& operator=(const DBConnectionFireBird& r);

  /** \brief Добавить бэкап точку перед операцией изменяющей
   *   состояние таблицы */
  mstatus_t addSavePoint();
  /**
   * \brief Шаблон функции оборачивающий операции с БД:
   *  1) Собрать запрос.
   *  2) Отправить и обработать результат.
   *  3) Обработать ошибки.
   * \param data данные для сборки запроса
   * \param res указатель на хранилище результата
   * \param setup_m функция сборки текста запроса
   * \param exec_m функция отправки запроса, парсинга результата
   * */
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
    if (firebird_work.IsAvailable() && sstr_len) {
      try {
        // execute query
        // todo: почему не возвращает статус?
        std::invoke(exec_m, *this, sstr, res);
        if (IS_DEBUG_MODE)
          Logging::Append(io_loglvl::debug_logs,
                          "Debug mode: \n\tЗапрос БД: " + sstr.str() + "\n\t");

      } catch (const FbException& e) {
        status_ = STATUS_HAVE_ERROR;
        if (utl != nullptr) {
          char buf[256];
          utl->formatStatus(buf, sizeof(buf), e.getStatus());
          error_.SetError(ERROR_DB_CONNECTION, buf);
        } else {
          error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
        }
      } catch (const std::exception& e) {
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION,
                        "Подключение к БД: exception. Запрос:\n" + sstr.str()
                            + "\nexception what: " + e.what());
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
  void execWithReturn(const std::stringstream& sstr, metadata_t* result);

  /** \brief Запрос создания метки сохранения */
  void execAddSavePoint(const std::stringstream& sstr, void*);
  /** \brief Запрос отката к метке сохранения */
  void execRollbackToSavePoint(const std::stringstream& sstr, void*);
  /** \brief Запрос существования таблицы */
  void execIsTableExists(const std::stringstream& sstr, bool* is_exists);
  /** \brief Запрос добавления колонки в таблицу */
  void execAddColumn(const std::stringstream& sstr, void*);
  /** \brief Запрос создания таблицы */
  void execCreateTable(const std::stringstream& sstr, void*);
  /** \brief Запрос удаления таблицы */
  void execDropTable(const std::stringstream& sstr, void*);

  /** \brief Запрос на добавление строки */
  void execInsert(const std::stringstream& sstr, metadata_t* result);
  /** \brief Запрос на удаление строки */
  void execDelete(const std::stringstream& sstr, void*);
  /** \brief Запрос выборки из таблицы
   * \note изменить 'void *' выход на 'pqxx::result *result' */
  void execSelect(const std::stringstream& sstr, metadata_t* result);
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
   * \brief Обёртка над параметрами работы с FireBird:
   *   поключения, транзакции
   * */
  struct _firebird_work : public BaseObject {
    // using namespace Firebird;
    // typedef std::pair<ITransaction *, ThrowStatusWraper &> transaction_pair;

   public:
    _firebird_work(const db_parameters& parameters)
        : BaseObject(STATUS_DEFAULT), parameters(parameters) {
      if (master != nullptr) {
        st = master->getStatus();
        prov = master->getDispatcher();
        utl = master->getUtilInterface();
        fb_status = std::make_unique<ThrowStatusWrapper>(st);
        status_ = STATUS_OK;
      } else {
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION,
                        "Инициализация мастера соединения");
      }
    }

    ~_firebird_work() { ReleaseConnection(); }
    /**
     * \brief Инициализировать параметры соединения, транзакции
     *
     * \param read_only Флаг подключения только на чтение
     * \throw FbException
     * \todo Наверное было бы интересно попробовать использовать coroutine
     * */
    bool InitConnection(bool read_only = false) {
      try {
        if (!read_only) {
          dpb = utl->getXpbBuilder(fb_status.get(), IXpbBuilder::DPB, NULL, 0);
          att = prov->attachDatabase(fb_status.get(), parameters.name.c_str(),
                                     0, NULL);
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
        tra = att->startTransaction(&fb_status, 0, NULL);
      } catch (const FbException& e) {
        status_ = STATUS_HAVE_ERROR;
        if (utl != nullptr) {
          char buf[256];
          utl->formatStatus(buf, sizeof(buf), e.getStatus());
          error_.SetError(ERROR_DB_CONNECTION, buf);
        } else {
          error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
        }
      } catch (const std::exception& e) {
        status_ = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION, e.what());
      }
      return IsAvailable();
    }
    /**
     * \brief Освободить параметры подключения
     * */
    void ReleaseConnection() {
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
      if (fb_status != nullptr)
        fb_status->dispose();
      if (prov != nullptr)
        prov->release();
    }
    /**
     * \brief Проверить установки текущей транзаккции
     * \todo Доделать
     * */
    bool IsAvailable() const { return is_status_ok(status_); }
    /**
     * \brief Указатель на транзакцию
     * */
    // transaction_pair GetTransaction() const { return transaction_pair(tra,
    // status); }
    /**
     * \brief Выслать на исполнение SQL команду
     *
     * \tparam SQLT Строковый тип SQL запроса
     * \param sql Текст запроса
     * \return mstatus_t Статус, как результат выполнения `execute`
     * */
    template <SQLType SQLT>
    mstatus_t execute(const SQLT& sql) {
      mstatus_t status = STATUS_NOT;
      try {
        if (att != nullptr) {
          if constexpr (std::is_same<SQLT, std::string>::value) {
            att->execute(fb_status.get(), tra, 0, sql.c_str(), SAMPLES_DIALECT,
                         NULL, NULL, NULL, NULL);
          } else if constexpr (std::is_same<SQLT, std::stringstream>::value) {
            att->execute(fb_status.get(), tra, 0, sql.string().c_str(),
                         SAMPLES_DIALECT, NULL, NULL, NULL, NULL);
          } else {
            att->execute(fb_status.get(), tra, 0, sql, SAMPLES_DIALECT, NULL,
                         NULL, NULL, NULL);
          }
          status = STATUS_OK;
        }
      } catch (const FbException& e) {
        status = STATUS_HAVE_ERROR;
        if (utl != nullptr) {
          char buf[256];
          utl->formatStatus(buf, sizeof(buf), e.getStatus());
          error_.SetError(ERROR_DB_CONNECTION, buf);
        } else {
          error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
        }
      } catch (const std::exception& e) {
        status = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION, e.what());
      }
      return status;
    }
    /**
     * \brief Подготовить на исполнение SQL команду
     *
     * \tparam SQLT Строковый тип SQL запроса
     * \param sql Текст запроса
     * \return mstatus_t Статус, как результат выполнения `prepare`
     * */
    template <SQLType SQLT>
    mstatus_t prepare(const SQLT& sql) {
      mstatus_t status = STATUS_NOT;
      try {
        if (att != nullptr) {
          if (stmt != nullptr) {
            stmt->free(fb_status.get());
            stmt = nullptr;
          }
          if constexpr (std::is_same<SQLT, std::string>::value) {
            stmt = att->prepare(fb_status.get(), tra, 0, sql.c_str(),
                                SAMPLES_DIALECT,
                                IStatement::PREPARE_PREFETCH_METADATA);
          } else if constexpr (std::is_same<SQLT, std::stringstream>::value) {
            stmt = att->prepare(fb_status.get(), tra, 0, sql.string().c_str(),
                                SAMPLES_DIALECT,
                                IStatement::PREPARE_PREFETCH_METADATA);
          } else {
            stmt = att->prepare(fb_status.get(), tra, 0, sql, SAMPLES_DIALECT,
                                IStatement::PREPARE_PREFETCH_METADATA);
          }
          // get list of columns
          meta = stmt->getOutputMetadata(fb_status.get());
          builder = meta->getBuilder(fb_status.get());
          unsigned cols = meta->getCount(fb_status.get());
          status = STATUS_OK;
        }
      } catch (const FbException& e) {
        status = STATUS_HAVE_ERROR;
        if (utl != nullptr) {
          char buf[256];
          utl->formatStatus(buf, sizeof(buf), e.getStatus());
          error_.SetError(ERROR_DB_CONNECTION, buf);
        } else {
          error_.SetError(ERROR_PAIR_DEFAULT(ERROR_DB_CONNECTION));
        }
      } catch (const std::exception& e) {
        status = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_CONNECTION, e.what());
      }
      return status;
    }
    /**
     * \brief Временная регистрация изменений в БД
     * */
    void commit() {
      if (tra != nullptr)
        tra->commit(fb_status.get());
    }
    /**
     * \brief Зафиксировать изменения в БД и отключиться от неё(закрыть
     * интерфейс)
     * */
    void commit_detach() {
      tra->commit(fb_status.get());
      att->detach(fb_status.get());
      tra = nullptr;
    }

   public:
    /**
     * \brief Указатель на master интерфейс, основной интерфейс firebird
     * */
    static IMaster* master;
    /**
     * \brief Параметры подключения к БД
     * */
    const db_parameters& parameters;
    /**
     * \brief Интерфейс объекта возврата(подробного описания ошибки)
     * */
    IStatus* st = nullptr;
    /**
     * \brief Интерфейс обеспечивающий подключение
     * */
    IProvider* prov = nullptr;
    //
    IUtil* utl = nullptr;
    /**
     * \brief Интерфейс методов работы с подключением
     * */
    IAttachment* att = nullptr;
    /**
     * \brief Интерфейс методов транзакций
     * */
    ITransaction* tra = nullptr;
    /**
     * \brief Интерфейс исполнения SQL запроса
     * */
    IStatement* stmt = nullptr;
    /**
     * \brief Интерфейс доступа к блокам параметров
     * */
    IXpbBuilder* dpb = nullptr;
    IMessageMetadata* meta = nullptr;
    IMetadataBuilder* builder = nullptr;
    /**
     * \brief Интерфейс получения данных операцией `SELECT`
     * */
    IResultSet* curs = nullptr;
    /**
     * \brief Объект-обёртка используемая для возвращения
     *   контекста операций с БД
     * */
    std::unique_ptr<ThrowStatusWrapper> fb_status = nullptr;
  } firebird_work;
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_CONNECTION_FIREBIRD_H_
