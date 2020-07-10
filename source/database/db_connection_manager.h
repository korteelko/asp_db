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
#include "db_connection.h"
#include "db_defines.h"
#include "db_query.h"
#include "db_tables.h"
#include "ErrorWrap.h"
#include "ThreadWrap.h"

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
  Transaction(DBConnection *connection);
  // хм, прикольно
  // Transaction (std::mutex &owner_mutex);
  void AddQuery(QuerySmartPtr &&query);
  mstatus_t ExecuteQueries();
  mstatus_t CancelTransaction();

  ErrorWrap GetError() const;
  mstatus_t GetResult() const;
  TransactionInfo GetInfo() const;

private:
  ErrorWrap error_;
  mstatus_t status_;
  /** \brief Указатель на подключение по которому
    *   будет осуществлена транзакция */
  DBConnection *connection_;
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


/** \brief Класс исключений модуля БД
  * \note Move this class to another file */
class DBException: public std::exception {
public:
  DBException(merror_t error, const std::string &msg);
  DBException(const std::string &msg);

  /* build options */
  /** \brief Добавить к исключению информацию о таблице */
  DBException &AddTableCode(db_table table);

  /** \brief Залогировать ошибку */
  void LogException();
  /** \brief Получить код ошибки */
  merror_t GetError() const;
  /** \brief Получить сообщение об ошибке */
  std::string GetErrorMessage() const;


private:
  /** \brief Ошибка, сюда же пишется сообщение */
  ErrorWrap error_;
  /** \brief Тип таблицы */
  db_table table_ = UNDEFINED_TABLE;
};


/** \brief Класс взаимодействия с БД, предоставляет API
  *   на все допустимые операции */
class DBConnectionManager {
public:
  DBConnectionManager(const IDBTables *tables);
  // API DB
  mstatus_t CheckConnection();
  // static const std::vector<std::string> &GetJSONKeys();
  /** \brief Попробовать законектится к БД */
  mstatus_t ResetConnectionParameters(const db_parameters &parameters);
  /** \brief Проверка существования таблицы */
  bool IsTableExists(db_table dt);
  /** \brief Создать таблицу */
  mstatus_t CreateTable(db_table dt);

  /* insert operations */
  /** \brief Сохранить в БД строку */
  template<class TableI>
  mstatus_t SaveSingleRow(TableI &ti, int *id_p = nullptr) {
    std::unique_ptr<db_query_insert_setup> dis(
        tables_->InitInsertSetup<TableI>({ti}));
    db_save_point sp("save_" + tables_->GetTableName<TableI>());
    id_container id_vec;
    mstatus_t st = exec_wrap<const db_query_insert_setup &, id_container,
        void (DBConnectionManager::*)(Transaction *, const db_query_insert_setup &,
        id_container *)>(*dis, &id_vec, &DBConnectionManager::saveRows, &sp);
    if (id_vec.id_vec.size() && id_p)
      *id_p = id_vec.id_vec[0];
    return st;
  }
  /** \brief Сохранить в БД вектор строк.
    * \todo replace with generic container */
  template<class TableI>
  mstatus_t SaveVectorOfRows(const std::vector<TableI> &tis,
      id_container *id_vec_p = nullptr) {
    std::unique_ptr<db_query_insert_setup> dis(
        tables_->InitInsertSetup<TableI>(tis));
    db_save_point sp("save_" + tables_->GetTableName<TableI>());
    mstatus_t st = exec_wrap<const db_query_insert_setup &, id_container,
        void (DBConnectionManager::*)(Transaction *, const db_query_insert_setup &,
        id_container *)>(*dis, id_vec_p, &DBConnectionManager::saveRows, &sp);
    return st;
  }

  /* select operations */
  /** \brief Вытащить из БД строки TableI по условиям из 'where' */
  template<class TableI, class ContainerT = std::vector<TableI>>
  mstatus_t SelectRows(TableI &where, ContainerT *res) {
    return selectData(tables_->GetTableCode<TableI>(), where, res);
  }
  /* table format */
  /** \brief Проверить формат таблицы */
  bool CheckTableFormat(db_table dt);
  /** \brief Обновить формат таблицы */
  mstatus_t UpdateTableFormat(db_table dt);

  template <class TableI>
  mstatus_t DeleteRows(TableI &where) {
    std::unique_ptr<db_query_delete_setup> dds(
        db_query_delete_setup::Init(tables_, tables_->GetTableCode<TableI>()));
    if (dds)
      dds->where_condition.reset(tables_->InitWhereTree<TableI>(where));
    db_save_point sp("delete_rows");
    return exec_wrap<const db_query_delete_setup &, void,
        void (DBConnectionManager::*)(Transaction *, const db_query_delete_setup &,
        void *)>(*dds, nullptr, &DBConnectionManager::deleteRows, &sp);
  }
  /** \brief Удалить строки */
  // mstatus_t DeleteModelInfo(model_info &where);

  /* rename method to GetError */
  merror_t GetErrorCode();
  mstatus_t GetStatus();
  /* remove this(GetErrorMessage), add LogIt */
  std::string GetErrorMessage();

private:
  class DBConnectionCreator;

private:
  /** \brief Обёртка над функционалом сбора и выполнения транзакции:
    *   подключение, создание точки сохранения
    * \param DataT data входные данные
    * \param OutT res указатель на выходные данные
    * \param SetupQueryF setup_m метод на добавление
    *   специализированного запроса(Query) к транзакции
    * \param sp_ptr указатель на сетап точки сохранения
    * \todo Слишком много шаблонных параметров получается,
    *   не очень красиво смотрится */
  template <class DataT, class OutT, class SetupQueryF>
  mstatus_t exec_wrap(DataT data, OutT *res, SetupQueryF setup_m,
      db_save_point *sp_ptr) {
    if (status_ == STATUS_DEFAULT)
      status_ = CheckConnection();
    mstatus_t trans_st;
    if (db_connection_ && is_status_aval(status_)) {
      Transaction tr(db_connection_.get());
      tr.AddQuery(QuerySmartPtr(
          new DBQuerySetupConnection(db_connection_.get())));
      // добавить точку сохранения, если есть необходимость
      if (sp_ptr)
        tr.AddQuery(QuerySmartPtr(
            new DBQueryAddSavePoint(db_connection_.get(), *sp_ptr)));
      // добавить специализированные запросы
      std::invoke(setup_m, *this, &tr, data, res);
      tr.AddQuery(QuerySmartPtr(
          new DBQueryCloseConnection(db_connection_.get())));
      try {
        trans_st = tryExecuteTransaction(tr);
      } catch (DBException &e) {
        e.LogException();
        trans_st = STATUS_HAVE_ERROR;
      } catch (std::exception &e) {
        trans_st = STATUS_HAVE_ERROR;
        error_.SetError(ERROR_DB_OPERATION, "Нерегламентированная ошибка" +
            std::string(e.what()));
      }
    } else {
      error_.SetError(ERROR_DB_CONNECTION, "Не удалось установить "
          "соединение для БД: " + parameters_.GetInfo());
      status_ = trans_st = STATUS_HAVE_ERROR;
    }
    return trans_st;
  }

  /** \brief Проинициализировать соединение с БД */
  void initDBConnection();

  /** \brief Собрать запрос на выборку данных */
  template <class DataT>
  mstatus_t selectData(db_table t, DataT &where, std::vector<DataT> *res) {
    std::unique_ptr<db_query_select_setup> dss(
        db_query_select_setup::Init(tables_, t));
    if (dss)
      dss->where_condition.reset(tables_->InitWhereTree<DataT>(where));
    db_query_select_result result(*dss);
    auto st = exec_wrap<const db_query_select_setup &, db_query_select_result,
        void (DBConnectionManager::*)(Transaction *, const db_query_select_setup &,
        db_query_select_result *)>(*dss, &result, &DBConnectionManager::selectRows,
        nullptr);
    if (st == STATUS_OK)
      tables_->SetSelectData(&result, res);
    return st;
  }

  /* добавить в транзакцию соответствующий запрос */
  /** \brief Запрос сушествования таблицы */
  void isTableExist(Transaction *tr, db_table dt, bool *is_exists);
  /** \brief Запрос создания таблицы */
  void createTable(Transaction *tr, db_table dt, void *);
  /** \brief Запрос получения формата таблицы */
  void getTableFormat(Transaction *tr, db_table dt,
      db_table_create_setup *exist_table);
  /** \brief Запрос сохранения строки данных */
  void saveRows(Transaction *tr, const db_query_insert_setup &qi,
      id_container *id_vec);
  /** \brief Запрос выборки параметров */
  void selectRows(Transaction *tr, const db_query_select_setup &qs,
      db_query_select_result *result);
  /** \brief Запрос на удаление рядов */
  void deleteRows(Transaction *tr, const db_query_delete_setup &qd, void *);

  /** \brief провести транзакцию tr из собраных запросов(строк) */
  [[nodiscard]]
  mstatus_t tryExecuteTransaction(Transaction &tr);

private:
  ErrorWrap error_;
  mstatus_t status_;
#ifdef CXX17
  /** \brief Мьютекс на подключение к БД */
  SharedMutex connect_init_lock_;
#else
  Mutex connect_init_lock_;
#endif  // CXX17
  /** \brief Параметры текущего подключения к БД */
  db_parameters parameters_;
  /** \brief */
  const IDBTables *tables_;
  /* todo: replace with connection pull */
  /** \brief Указатель иницианилизированное подключение */
  std::unique_ptr<DBConnection> db_connection_;
};


/** \brief Закрытый класс создания соединений с БД */
class DBConnectionManager::DBConnectionCreator {
  friend class DBConnectionManager;

private:
  DBConnectionCreator();

  DBConnection *InitDBConnection(const IDBTables *tables,
      const db_parameters &parameters);
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_CONNECTION_MANAGER_H_
