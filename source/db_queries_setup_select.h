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
#include "db_where.h"

#include <optional>

namespace asp_db {
/**
 * \brief Структура для сборки SELECT(и DELETE) запросов
 *
 * \throw idbtables_exception В случае невалидного состояния дерева
 *   WhereTree(ошибка разработки и проектирования)
 * */
struct db_query_select_setup : public db_query_basesetup {
 public:
  /**
   * \brief Статический конструктор для select запросов с where условиями
   * */
  template <db_table table>
  static std::shared_ptr<db_query_select_setup> Init(WhereTree<table>& wt) {
    auto tables = wt.GetTables();
    if (tables == nullptr)
      throw idbtables_exception<table>(
          "Объект WhereTree не содержит информации о пространстве таблиц");
    return std::shared_ptr<db_query_select_setup>(new db_query_select_setup(
        table, *tables->GetFieldsCollection(table), wt.GetWhereTree(), false));
  }
  /**
   * \brief Статический конструктор для select запросов всех данных
   * */
  static std::shared_ptr<db_query_select_setup> Init(const IDBTables* tables,
                                                     db_table table,
                                                     bool act2all) {
    return std::shared_ptr<db_query_select_setup>(new db_query_select_setup(
        table, *tables->GetFieldsCollection(table), nullptr, act2all));
  }

  virtual ~db_query_select_setup() = default;
  /**
   * \brief Получить строку запроса where
   *
   * \return Строку where условия или nullObject, если DBWhereClause
   *   не проинициализировано(если оно пустое, то вернёт пустую строку)
   * */
  std::optional<std::string> GetWhereString() const;

  /**
   * \brief Установлен ли флаг применения ко всем данным
   *
   * \return Значение флага применения ко всем данным
   * */
  bool IsActToAll() const { return act2all_; }

 protected:
  db_query_select_setup(
      db_table _table,
      const db_fields_collection& _fields,
      const std::shared_ptr<DBWhereClause<where_node_data>>& where,
      bool act2all);

 protected:
  /**
   * \brief Сетап выражения where для SELECT/UPDATE/DELETE запросов
   * */
  const std::shared_ptr<DBWhereClause<where_node_data>> where_ = nullptr;
  /**
   * \brief Применить операцию ко всемданным таблицы
   * */
  const bool act2all_;
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
