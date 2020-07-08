/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_tables *
 *   Апишечка на взаимодействие функционала модуля базы данных
 * с представлением конкретных таблиц
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_TABLES_H_
#define _DATABASE__DB_TABLES_H_

#include "db_defines.h"
#include "db_queries_setup.h"

#include <string>
#include <vector>

#include <stdint.h>


namespace asp_db {
#define UNDEFINED_TABLE         0x00000000
#define UNDEFINED_COLUMN        0x00000000

/** \brief Интерфейс объекта-связи для модуля базы данных */
class IDBTables {
public:
  virtual ~IDBTables() {}

  /** \brief */
  virtual std::string GetTableName(db_table t) const = 0;
  /** \brief */
  virtual const db_fields_collection *GetFieldsCollection(db_table t) const = 0;
  /** \brief */
  virtual db_table StrToTableCode(const std::string &tname) const = 0;
  /** \brief */
  virtual std::string GetIdColumnName(db_table dt) const = 0;
  /** \brief */
  //virtual db_ref_collection RefCollectionByCode(db_table table) const = 0;
  virtual const db_table_create_setup &CreateSetupByCode(db_table dt) const = 0;

  /** \brief Шаблон функции получения имени таблицы по типу.
    *   Шаблон специализируется классами используемых
    *   таблиц в основной программе */
  template <class T>
  std::string GetTableName() const { return ""; }
  /** \brief Шаблон функции получения кода таблицы по типу.
    *   Шаблон специализируется классами используемых
    *   таблиц в основной программе */
  template <class T>
  db_table GetTableCode() const { return UNDEFINED_TABLE; }

  /** \brief */
  template <class TableI>
  db_query_insert_setup *InitInsertSetup(
      const std::vector<TableI> &insert_data) const {
    if (db_query_insert_setup::haveConflict(insert_data))
      return nullptr;
    db_table table = GetTableCode<TableI>();
    db_query_insert_setup *ins_setup = new db_query_insert_setup(table,
        *GetFieldsCollection(table));
    if (ins_setup)
      for (auto &x: insert_data)
        setInsertValues<TableI>(ins_setup, x);
    return ins_setup;
  }

  /** \brief */
  template <class TableI>
  void SetSelectData(db_query_select_result *src,
      std::vector<TableI> *out_vec) const;

  template <class TableI>
  db_where_tree *InitWhereTree(const TableI &where) const {
    std::unique_ptr<db_query_insert_setup> qis(
        InitInsertSetup<TableI>({where}));
    return db_where_tree::init(qis.get());
  }

protected:
  /** \brief Шаблонная функция сбора insert_setup по структуре БД
    * \note Шаблон закрыт, разрешены только спецификации ниже */
  template <class T>
  void setInsertValues(db_query_insert_setup *src, const T &select_data) const;

  template <class DataInfo, class Table>
  db_query_insert_setup *init(Table t, db_query_insert_setup *src,
      const std::vector<DataInfo> &insert_data) {
    if (haveConflict(insert_data))
      return nullptr;
    db_query_insert_setup *ins_setup = new db_query_insert_setup(
        t, *GetFieldsCollection(t));
    if (ins_setup)
      for (const auto &x: insert_data)
        setInsertValues<Table>(src, x);
    return ins_setup;
  }
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_TABLES_H_
