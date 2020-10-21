/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_queries_setup  *
 *   Здесь прописана логика создания абстрактных запросов к БД,
 * которые создаются классом управляющим подключением к БД
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_QUERIES_SETUP_H_
#define _DATABASE__DB_QUERIES_SETUP_H_

#include "Common.h"
#include "ErrorWrap.h"
#include "db_defines.h"
#include "db_expression.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace asp_db {
class IDBTables;
/**
 * \brief Сетап для добавления точки сохранения
 * */
struct db_save_point {
 public:
  /**
   * \brief Формат имени точки сохранения:
   *  [a-z,A-Z,_]{1}[a-z,A-Z,1-9,_]{*}
   * */
  db_save_point(const std::string& _name);
  /**
   * \brief Получить собранную строку для добавления точки сохранения
   * */
  std::string GetString() const;

 public:
  /**
   * \brief Собственное уникальное имя ноды
   * */
  std::string name;
};

/**
 * \brief Сетап удаления таблицы
 * */
struct db_table_drop_setup {
  db_table table;
  db_reference_act act;
};

/* Data structs */
/** \brief Структура создания таблицы БД и, в перспективе,
 *   формата существующей таблицы
 * \note Эту же структуру можно заполнять по данным полученным от СУБД,
 *   результаты же можно сравнить форматы и обновить существующую таблицу
 *   не удаляя её */
struct db_table_create_setup {
  /** \brief Набор имён полей таблицы, составляющих уникальный комплекс */
  typedef std::vector<std::string> unique_constrain;
  /** \brief Контейнер уникальных комплексов */
  typedef std::vector<unique_constrain> uniques_container;
  /** \brief enum для сравнений сетапов */
  enum compare_field {
    /** \brief Код таблицы */
    cf_table = 0,
    /** \brief Поля таблицы */
    cf_fields,
    /** \brief Сложный первичный ключ */
    cf_pk_string,
    /** \brief Уникальные комплексы */
    cf_unique_constrains,
    /** \brief Внешние ссылки */
    cf_ref_strings
  };

 public:
  /** \brief Конструктор для записи данных из БД */
  db_table_create_setup(db_table table);
  /** \brief Конструктор для добавления таблицы в БД */
  db_table_create_setup(db_table table,
                        const db_fields_collection& fields,
                        const uniques_container& unique_constrains,
                        const std::shared_ptr<db_ref_collection>& ref_strings);

  /** \brief Сравнить сетапы таблиц */
  std::map<compare_field, bool> Compare(const db_table_create_setup& r);

  /** \brief Проверить ссылки */
  void CheckReferences(const IDBTables* tables);

 public:
  ErrorWrap error;
  /** \brief Код таблицы */
  db_table table;
  /** \brief Наборы полей таблицы */
  db_fields_collection fields;
  /** \brief Вектор имен полей таблицы которые составляют
   *   сложный(неодинарный) первичный ключ */
  db_complex_pk pk_string;
  /** \brief Наборы уникальных комплексов */
  uniques_container unique_constrains;
  /** \brief Набор внешних ссылок */
  std::shared_ptr<db_ref_collection> ref_strings = nullptr;

 private:
  /** \brief Собрать поле 'pk_string' */
  void setupPrimaryKeyString();
  /** \brief Шаблон сравения массивов */
  template <class ArrayT>
  static bool IsSame(const ArrayT& l, const ArrayT& r) {
    bool same = l.size() == r.size();
    if (same) {
      size_t i = 0;
      for (; i < l.size(); ++i)
        if (l[i] != r[i])
          break;
      if (i == l.size())
        same = true;
    }
    return same;
  }
};

/* queries setup */
/** \brief Базовая структура сборки запроса */
struct db_query_basesetup {
  typedef size_t field_index;
  /** \brief Набор данных */
  typedef std::map<field_index, std::string> row_values;

  static constexpr size_t field_index_end = std::string::npos;

 public:
  virtual ~db_query_basesetup() = default;

  field_index IndexByFieldId(db_variable_id fid);

 protected:
  db_query_basesetup(db_table table, const db_fields_collection& fields);

 public:
  ErrorWrap error;
  db_table table;

 public:
  /** \brief Ссылка на коллекцию полей(столбцов)
   *   таблицы в БД для таблицы 'table' */
  const db_fields_collection& fields;
};

/**
 * \brief Структура для сборки INSERT запросов
 * */
struct db_query_insert_setup : public db_query_basesetup {
 public:
  /**
   * \brief Проверить соответствие значений полей initialized в векторе
   *   элементов данных выборки. Для всех должны быть одинаковы
   * \note Функция для контейнера объектов, не указателей
   * */
  template <class DataInfo>
  static bool haveConflict(const std::vector<DataInfo>& select_data) {
    if (!select_data.empty()) {
      auto initialized = (*select_data.begin()).initialized;
      for (auto it = select_data.begin() + 1; it != select_data.end(); ++it)
        if (initialized != (*it).initialized)
          return true;
    }
    return false;
  }

  db_query_insert_setup(db_table _table, const db_fields_collection& _fields);
  virtual ~db_query_insert_setup() = default;

  size_t RowsSize() const;
  /**
   * \brief Функция собирающая обычное дерево условий `a` = 'A',
   *   разнесённых операторами AND
   * */
  std::shared_ptr<DBWhereClause<where_node_data>> InitInsertTree();

 public:
  /**
   * \brief Набор значений для операций INSERT|SELECT|DELETE
   * */
  std::vector<row_values> values_vec;
};

/**
 * \brief Контейнер для результатов операции INSERT, иначе говоря,
 *   для полученных от СУБД идентификаторов рядов/строк
 * */
struct id_container {
 public:
  mstatus_t status = STATUS_DEFAULT;
  /**
   * \brief Контенер идентификаторов строк вектора в БД
   * */
  std::vector<int> id_vec;
};

/**
 * \brief структура для сборки SELECT(и DELETE) запросов
 * */
struct db_query_select_setup : public db_query_basesetup {
 public:
  static db_query_select_setup* Init(const IDBTables* tables, db_table _table);

  virtual ~db_query_select_setup() = default;

 public:
  /**
   * \brief Сетап выражения where для SELECT/UPDATE/DELETE запросов
   * */
  std::shared_ptr<DBWhereClause<where_node_data>> where_condition;

 protected:
  db_query_select_setup(db_table _table, const db_fields_collection& _fields);
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
 * \brief Структура для сборки INSERT запросов
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

#endif  // !_DATABASE__DB_QUERIES_SETUP_H_
