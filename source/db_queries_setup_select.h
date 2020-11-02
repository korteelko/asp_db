/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_queries_setup_select *
 *   Модуль сборки запросов выборки, удаления, обновления таблиц
 * данных в БД
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_QUERIES_SETUP_SELECT_H_
#define _DATABASE__DB_QUERIES_SETUP_SELECT_H_

#include "db_expression.h"
#include "db_queries_setup.h"

#include <optional>

namespace asp_db {
/**
 * \brief Структура для сборки SELECT(и DELETE) запросов
 * */
struct db_query_select_setup : public db_query_basesetup {
 public:
  static db_query_select_setup* Init(const IDBTables* tables, db_table _table);

  virtual ~db_query_select_setup() = default;
  /**
   * \brief Попробовать сменить выражение условий
   * */
  void ResetWhereClause(
      const std::shared_ptr<DBWhereClause<where_node_data>>& where);
  /**
   * \brief Получить строку запроса where
   *
   * \return Строку where условия или nullObject, если DBWhereClause
   * не проинициализировано(если оно пустое, то вернёт пустую строку)
   * */
  std::optional<std::string> GetWhereString() const;

  bool IsActToAll() const { return act2all_; }
  void SetActAll(bool value) { act2all_ = value; }

 protected:
  db_query_select_setup(db_table _table, const db_fields_collection& _fields);

 protected:
  /**
   * \brief Сетап выражения where для SELECT/UPDATE/DELETE запросов
   * */
  std::shared_ptr<DBWhereClause<where_node_data>> where_ = nullptr;
  /**
   * \brief Применить операцию ко всемданным таблицы
   * */
  bool act2all_ = false;
};
/**
 * \brief псевдоним DELETE запросов
 * */
typedef db_query_select_setup db_query_delete_setup;

/**
 * \brief Структура для сборки UPDATE запросов
 * */
struct db_query_update_setup : public db_query_select_setup {
 public:
  static db_query_update_setup* Init(db_table _table);

 public:
  row_values values;

 protected:
  db_query_update_setup(db_table _table, const db_fields_collection& _fields);
};

/* select result */
/**
 * \brief Структура для сборки ответов на запросы
 * */
struct db_query_select_result : public db_query_basesetup {
 public:
  db_query_select_result() = delete;
  db_query_select_result(const db_query_select_setup& setup);

  virtual ~db_query_select_result() = default;

  /**
   * \brief Проверить соответствие строки strname и имени поля
   * \note todo: Заменить строки на int идентификаторы
   * */
  bool isFieldName(const std::string& strname, const db_variable& var);

 public:
  /**
   * \brief Набор значений для операций INSERT/UPDATE
   * */
  std::vector<row_values> values_vec;
};
inline bool db_query_select_result::isFieldName(const std::string& strname,
                                                const db_variable& var) {
  return strname == var.fname;
}
}  // namespace asp_db

#endif  // !_DATABASE__DB_QUERIES_SETUP_SELECT_H_
