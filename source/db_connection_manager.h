/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_connection_manager *
 *   Класс управляющий подключением к базе данных
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_CONNECTION_MANAGER_H_
#define _DATABASE__DB_CONNECTION_MANAGER_H_

#include "Common.h"
#include "ErrorWrap.h"
#include "Logging.h"
#include "ThreadWrap.h"
#include "db_connection.h"
#include "db_defines.h"
#include "db_queries_setup.h"
#include "db_queries_setup_select.h"
#include "db_query.h"
#include "db_tables.h"

#include <exception>
#include <functional>
#include <string>
#include <vector>

#include <stdint.h>

namespace asp_db {
/** \brief Класс инкапсулирующий конечную высокоуровневую операцию с БД
 * \note Определения 'Query' и 'Transaction' в программе условны:
 *   Query - примит обращения к БД, Transaction - связный набор примитивов
 * \todo Переместить класс в другой файл */
class Transaction {
 public:
  class TransactionInfo;

 public:
  Transaction(DBConnection* connection);
  // хм, прикольно
  // Transaction (std::mutex &owner_mutex);
  void AddQuery(QuerySmartPtr&& query);
  mstatus_t ExecuteQueries();
  mstatus_t CancelTransaction();

  ErrorWrap GetError() const;
  mstatus_t GetResult() const;
  TransactionInfo GetInfo() const;
  DBConnection* GetConnection() const;

 private:
  ErrorWrap error_;
  mstatus_t status_;
  /** \brief Указатель на подключение по которому
   *   будет осуществлена транзакция */
  DBConnection* connection_;
  /** \brief Очередь простых запросов, составляющих полную транзакцию */
  QueryContainer queries_;
};

/** \brief Класс инкапсулирующий информацию о транзакции - лог, результат
 * \note Не доделан */
class Transaction::TransactionInfo {
  friend class Transaction;
  // date and time +
  // info
 public:
  std::string GetInfo();

 private:
  TransactionInfo();

 private:
  std::string info_;
};

/**
 * \brief Класс исключений модуля БД
 * \note Move this class to another file
 * */
class DBException : public std::exception {
 public:
  DBException(merror_t error, const std::string& msg);
  DBException(const std::string& msg);

  const char* what() const noexcept;

  /* build options */
  /**
   * \brief Добавить к исключению информацию о таблице
   * */
  DBException& AddTableCode(db_table table);

  /**
   * \brief Залогировать ошибку
   * */
  void LogException();
  /**
   * \brief Получить код ошибки
   * */
  merror_t GetError() const;
  /**
   * \brief Получить сообщение об ошибке
   * */
  std::string GetErrorMessage() const;

 private:
  /**
   * \brief Ошибка, сюда же пишется сообщение
   * */
  ErrorWrap error_;
  /**
   * \brief Код таблицы
   * */
  db_table table_ = UNDEFINED_TABLE;
};

/**
 * \brief Класс взаимодействия с БД, предоставляет API
 *   на все допустимые операции
 *
 * В целом класс представляет собой фасад на подключение к БД
 * */
class DBConnectionManager {
 public:
  DBConnectionManager(const IDBTables* tables);
  // API DB
  mstatus_t CheckConnection();
  // static const std::vector<std::string> &GetJSONKeys();
  /** \brief Попробовать законектится к БД */
  mstatus_t ResetConnectionParameters(const db_parameters& parameters);
  /** \brief Проверка существования таблицы */
  bool IsTableExists(db_table dt);
  /** \brief Создать таблицу */
  mstatus_t CreateTable(db_table dt);

  /* insert operations */
  /** \brief Сохранить в БД строку */
  template <class TableI>
  mstatus_t SaveSingleRow(TableI& ti, int* id_p = nullptr);
  /**
   * \brief Сохранить в БД вектор строк.
   * \todo replace with generic container
   * */
  template <class TableI>
  mstatus_t SaveVectorOfRows(const std::vector<TableI>& tis,
                             id_container* id_vec_p = nullptr);
  /**
   * \brief Сохранить в БД строки ещё не добавленные.
   * \todo replace with generic container
   * */
  template <class TableI>
  mstatus_t SaveNotExistsRows(
      const std::vector<TableI>& tis,
      id_container* id_vec_p = nullptr,
      insert_on_exists_act on_exists = insert_on_exists_act::do_nothing);
  /* select operations */
  /**
   * \brief Вытащить из БД строки TableI по условиям из 'where',
   *   результаты записать в вектор res
   *
   * \tparam table Идентификатор таблицы
   * \tparam TableI Структура, реализующая таблицу данных
   *
   * \param where Дерево условий выборки
   * \param res Вектор выходных результатов в формате структуры,
   *   реализующей таблицу данных
   *
   * \return Статус выполнения команды
   * */
  template <db_table table, class TableI>
  mstatus_t SelectRows(WhereTree<table>& where, std::vector<TableI>* res) {
    std::shared_ptr<db_query_select_setup> dss(
        db_query_select_setup::Init(where));
    return selectRowsImp<TableI>(dss, res);
  }
  /**
   * \brief Вытащить из БД все строки TableI
   *
   * \tparam TableI Структура, реализующая таблицу данных
   *
   * \param table Идентификатор таблицы
   * \param res Вектор выходных результатов в формате структуры,
   *   реализующей таблицу данных
   *
   * \return Статус выполнения команды
   * */
  template <class TableI>
  mstatus_t SelectAllRows(db_table table, std::vector<TableI>* res) {
    auto dss = db_query_select_setup::Init(tables_, table, true);
    return selectRowsImp<TableI>(dss, res);
  }
  /**
   * \brief Удалить строки таблицы соответствующие инициализированным
   *   в аргументе метода - объекте where
   *
   * \tparam table Идентификатор таблицы
   *
   * \param where Дерево WHERE условий
   *
   * \return Результат выполнения операции
   * */
  template <db_table table>
  mstatus_t DeleteRows(WhereTree<table>& where) {
    std::shared_ptr<db_query_delete_setup> dds(
        db_query_delete_setup::Init(where));
    return deleteRowsImp(dds);
  }
  /**
   * \brief Удалить все строки таблицы `table`
   *
   * \param table Идентификатор таблицы
   * */
  mstatus_t DeleteAllRows(db_table table);

  /* table format */
  /**
   * \brief Сравнить заданную в программе конфигурацию
   *   таблицы с найденной в СУБД
   * */
  bool CheckTableFormat(db_table dt);
  /**
   * \brief Обновить формат таблицы
   * */
  mstatus_t UpdateTableFormat(db_table dt);

  mstatus_t GetStatus();
  merror_t GetError();
  std::string GetErrorMessage();

 private:
  class DBConnectionCreator;
  typedef DBConnectionManager::DBConnectionCreator ConnectionCreator;

 private:
  template <class TableI>
  mstatus_t selectRowsImp(std::shared_ptr<db_query_select_setup>& dss,
                          std::vector<TableI>* res) {
    db_query_select_result result(*dss);
    auto st = exec_wrap<const db_query_select_setup&, db_query_select_result,
                        void (DBConnectionManager::*)(
                            Transaction*, const db_query_select_setup&,
                            db_query_select_result*)>(
        *dss, &result, &DBConnectionManager::selectRows, nullptr);
    if (is_status_ok(st))
      tables_->SetSelectData(&result, res);
    return st;
  }
  /**
   * assert
   * */
  template <class TableI>
  mstatus_t saveRowsImp(const db_query_insert_setup& dis,
                        id_container* id_vec_p);
  mstatus_t deleteRowsImp(const std::shared_ptr<db_query_delete_setup>& dds);
  /**
   * \brief Обёртка над функционалом сбора и выполнения транзакции:
   *   подключение, создание точки сохранения
   * \param DataT data входные данные
   * \param OutT res указатель на выходные данные
   * \param SetupQueryF setup_m метод на добавление
   *   специализированного запроса(Query) к транзакции
   * \param sp_ptr указатель на сетап точки сохранения
   *
   * \todo Слишком много шаблонных параметров получается,
   *   не очень красиво смотрится
   * */
  template <class DataT, class OutT, class SetupQueryF>
  mstatus_t exec_wrap(DataT data,
                      OutT* res,
                      SetupQueryF setup_m,
                      db_save_point* sp_ptr);
  /**
   * \brief Проинициализировать соединение с БД
   * */
  void initDBConnection();

  /* добавить в транзакцию соответствующий запрос */
  /** \brief Запрос сушествования таблицы */
  void isTableExist(Transaction* tr, db_table dt, bool* is_exists);
  /** \brief Запрос создания таблицы */
  void createTable(Transaction* tr, db_table dt, void*);
  /** \brief Запрос получения формата таблицы */
  void getTableFormat(Transaction* tr,
                      db_table dt,
                      db_table_create_setup* exist_table);
  /** \brief Запрос сохранения строки данных */
  void saveRows(Transaction* tr,
                const db_query_insert_setup& qi,
                id_container* id_vec);
  /** \brief Запрос выборки параметров */
  void selectRows(Transaction* tr,
                  const db_query_select_setup& qs,
                  db_query_select_result* result);
  /** \brief Запрос на удаление рядов */
  void deleteRows(Transaction* tr, const db_query_delete_setup& qd, void*);

  /** \brief провести транзакцию tr из собраных запросов(строк) */
  [[nodiscard]] mstatus_t tryExecuteTransaction(Transaction& tr);

 private:
  ErrorWrap error_;
  mstatus_t status_;
  /**
   * \brief Мьютекс на подключение к БД
   * */
  SharedMutex connect_init_lock_;
  /**
   * \brief Параметры текущего подключения к БД
   * */
  db_parameters parameters_;
  /**
   * \brief Указатель на С++ интерфейс имплементации таблиц БД
   * */
  const IDBTables* tables_;
  /* todo: replace with connection pull */
  /**
   * \brief Указатель иницианилизированное подключение
   * \note Мэйби контейнер??? Хотя лучше несколько мэнеджеров держать
   * */
  std::unique_ptr<DBConnection> db_connection_;
};

/**
 * \brief Закрытый класс создания соединений с БД
 * */
class DBConnectionManager::DBConnectionCreator {
  OWNER(DBConnectionManager);

 private:
  DBConnectionCreator();

  /**
   * \brief Инициализировать соединение с БД
   * \param tables Указатель на класс реализующий
   *   операции с таблицами БД
   * \param parameters Параметры подключения
   *
   * \return Указатель на реализацию подключения или nullptr
   * */
  std::unique_ptr<DBConnection> initDBConnection(
      const IDBTables* tables,
      const db_parameters& parameters);
  /**
   * \brief Создать копию оригинального соединения для
   *   организации параллельных запросов к БД
   * \param orig Указатель на оригинал подключения
   *
   * \return Указатель на реализацию подключения или nullptr
   * */
  static std::shared_ptr<DBConnection> cloneConnection(DBConnection* orig);

 private:
  /**
   * \brief Логгер модуля БД
   *
   * \todo why it is static?
   * */
  static PrivateLogging db_logger_;
};

/* template methods of DBConnectionManager */
template <class TableI>
mstatus_t DBConnectionManager::SaveSingleRow(TableI& ti, int* id_p) {
  std::unique_ptr<db_query_insert_setup> dis(
      tables_->InitInsertSetup<TableI>({ti}));
  id_container id_vec;
  mstatus_t st = STATUS_NOT;
  if (dis.get() != nullptr) {
    st = saveRowsImp<TableI>(*dis, &id_vec);
  } else {
    Logging::Append(io_loglvl::warn_logs,
                    "Ошибка добавления набора строк в SaveSingleRow \n"
                    "к БД - не инициализирован сетап добавляемыхх данных");
  }
  if (id_vec.id_vec.size() && id_p)
    *id_p = id_vec.id_vec[0];
  return st;
}
template <class TableI>
mstatus_t DBConnectionManager::SaveVectorOfRows(const std::vector<TableI>& tis,
                                                id_container* id_vec_p) {
  std::unique_ptr<db_query_insert_setup> dis(
      tables_->InitInsertSetup<TableI>(tis));
  mstatus_t st = STATUS_NOT;
  if (dis.get() != nullptr) {
    st = saveRowsImp<TableI>(*dis, id_vec_p);
  } else {
    Logging::Append(io_loglvl::warn_logs,
                    "Ошибка добавления набора строк в SaveVectorOfRows \n"
                    "к БД - не инициализирован сетап добавляемыхх данных");
  }
  return st;
}
template <class TableI>
mstatus_t DBConnectionManager::SaveNotExistsRows(
    const std::vector<TableI>& tis,
    id_container* id_vec_p,
    insert_on_exists_act on_exists) {
  mstatus_t st = STATUS_NOT;
  std::unique_ptr<db_query_insert_setup> dis(
      tables_->InitInsertSetup<TableI>(tis));
  if (dis.get() != nullptr) {
    dis->SetOnExistAct(on_exists);
    st = saveRowsImp<TableI>(*dis, id_vec_p);
  } else {
    Logging::Append(io_loglvl::warn_logs,
                    "Ошибка добавления набора строк в SaveNotExistsRows"
                    "к БД - не инициализирован сетап добавляемыхх данных");
  }
  return st;
}

template <class TableI>
mstatus_t DBConnectionManager::saveRowsImp(const db_query_insert_setup& dis,
                                           id_container* id_vec_p) {
  db_save_point sp("save_" + tables_->GetTableName<TableI>());
  return exec_wrap<const db_query_insert_setup&, id_container,
                   void (DBConnectionManager::*)(Transaction*,
                                                 const db_query_insert_setup&,
                                                 id_container*)>(
      dis, id_vec_p, &DBConnectionManager::saveRows, &sp);
}

template <class DataT, class OutT, class SetupQueryF>
mstatus_t DBConnectionManager::exec_wrap(DataT data,
                                         OutT* res,
                                         SetupQueryF setup_m,
                                         db_save_point* sp_ptr) {
  if (status_ == STATUS_DEFAULT)
    status_ = CheckConnection();
  mstatus_t trans_st = STATUS_NOT;
  if (db_connection_ && is_status_aval(status_)) {
    auto c = DBConnectionCreator().cloneConnection(db_connection_.get());
    if (c.get()) {
      Transaction tr(c.get());
      tr.AddQuery(QuerySmartPtr(new DBQuerySetupConnection(c.get())));
      // добавить точку сохранения, если есть необходимость
      if (sp_ptr)
        tr.AddQuery(QuerySmartPtr(new DBQueryAddSavePoint(c.get(), *sp_ptr)));
      // добавить специализированные запросы
      std::invoke(setup_m, *this, &tr, data, res);
      tr.AddQuery(QuerySmartPtr(new DBQueryCloseConnection(c.get())));
      try {
        trans_st = tryExecuteTransaction(tr);
      } catch (DBException& e) {
        e.LogException();
        trans_st = STATUS_HAVE_ERROR;
      } catch (std::exception& e) {
        trans_st = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_OPERATION,
                        "Нерегламентированная ошибка" + std::string(e.what()));
      }
    }
  } else {
    error_.SetError(ERROR_DB_CONNECTION,
                    "Не удалось установить "
                    "соединение для БД: "
                        + parameters_.GetInfo());
    status_ = trans_st = STATUS_HAVE_ERROR;
  }
  return trans_st;
}
}  // namespace asp_db

#endif  // !_DATABASE__DB_CONNECTION_MANAGER_H_
