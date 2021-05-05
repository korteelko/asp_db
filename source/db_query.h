/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_QUERY_H_
#define _DATABASE__DB_QUERY_H_

#include "asp_utils/ErrorWrap.h"
#include "db_defines.h"
#include "db_queries_setup.h"
#include "db_queries_setup_select.h"

#include <memory>
#include <string>
#include <vector>

namespace asp_db {
class DBConnection;
struct db_table_create_setup;

/**
 * \brief абстрактный класс запросов
 * */
class DBQuery {
 public:
  /** \brief Выполнение запроса */
  bool IsPerformed() const;

  /* todo: rename to 'LogDataBaseError' or something */
  void LogDBConnectionError();

  /** \brief Обёртка над функцией исполнения команды */
  virtual mstatus_t Execute();
  virtual void unExecute();
  virtual ~DBQuery();

 protected:
  DBQuery(DBConnection* db_ptr);
  /** \brief Функция исполнения команды */
  virtual mstatus_t exec() = 0;
  virtual std::string q_info() = 0;

 protected:
  // std::string query_body_;
  mstatus_t status_;
  /** \brief Указатель на подключение к БД */
  DBConnection* db_ptr_;
  /** \brief Выполнение запроса */
  bool is_performed_;
};
typedef std::shared_ptr<DBQuery> QuerySmartPtr;
/* todo: rename QueryContainer to QuerySequence */
typedef std::vector<QuerySmartPtr> QueryContainer;

/**
 * \brief Запрос установить соединение с бд
 * */
class DBQuerySetupConnection : public DBQuery {
 public:
  DBQuerySetupConnection(DBConnection* db_ptr);
  /** \brief отключиться от бд */
  void unExecute() override;

 protected:
  mstatus_t exec() override;
  std::string q_info() override;
};

/**
 * \brief Запрос отключения от бд
 * */
class DBQueryCloseConnection : public DBQuery {
 public:
  DBQueryCloseConnection(DBConnection* db_ptr);
  mstatus_t Execute() override;

 protected:
  mstatus_t exec() override;
  std::string q_info() override;
};

/**
 * \brief Запрос создания точки сохранения
 * */
class DBQueryAddSavePoint : public DBQuery {
 public:
  DBQueryAddSavePoint(DBConnection* ptr, const db_save_point& sp);
  /** \brief Rollback к этой точке сохранения */
  void unExecute() override;

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_save_point& save_point;
};

/**
 * \brief Запрос проверки существования таблицы в бд
 * */
class DBQueryIsTableExists : public DBQuery {
 public:
  DBQueryIsTableExists(DBConnection* db_ptr, db_table dt, bool& is_exists);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  db_table table_;
  bool& is_exists_;
};

/**
 * \brief Запрос создания таблицы в бд
 * */
class DBQueryCreateTable : public DBQuery {
 public:
  DBQueryCreateTable(DBConnection* db_ptr,
                     const db_table_create_setup& create_setup);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_table_create_setup& create_setup;
};

/**
 * \brief Запрос обновления формата таблицы в бд
 * \note Обновление таблицы БД
 * */
class DBQueryUpdateTable : public DBQuery {
 public:
  DBQueryUpdateTable(DBConnection* db_ptr,
                     const db_table_create_setup& table_setup);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_table_create_setup& update_setup;
};

/**
 * \brief Запрос на добавление строки
 * */
class DBQueryInsertRows : public DBQuery {
 public:
  DBQueryInsertRows(DBConnection* db_ptr,
                    const db_query_insert_setup& insert_setup,
                    id_container* id_vec);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_query_insert_setup& insert_setup;
  /** \brief Указатель на вектор присвоенных строкам id */
  id_container* id_vec;
};

/**
 * \brief Запрос выборки
 * */
class DBQuerySelectRows : public DBQuery {
 public:
  DBQuerySelectRows(DBConnection* db_ptr,
                    const db_query_select_setup& select_setup,
                    db_query_select_result* result);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_query_select_setup& select_setup;
  db_query_select_result* result;
};

/**
 * \brief Запрос на удаление рядов из БД
 * */
class DBQueryDeleteRows : public DBQuery {
 public:
  DBQueryDeleteRows(DBConnection* db_ptr,
                    const db_query_delete_setup& delete_setup);

 protected:
  mstatus_t exec() override;
  std::string q_info() override;

 private:
  const db_query_delete_setup& delete_setup;
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_QUERY_H_
