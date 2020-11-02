/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_where *
 *   Апишечка на создание where выражений
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_WHERE_H_
#define _DATABASE__DB_WHERE_H_

#include "db_expression.h"
#include "db_tables.h"

namespace asp_db {
namespace wns = where_nodes;
/**
 * \brief Класс сборки дерева условий where запроса
 *
 * \tparam table Идентификатор таблицы под которую собирается where запрос
 * */
template <db_table table>
class WhereTreeSetup {
 public:
  WhereTreeSetup(IDBTables* tables) : tables_(tables) {}

  /* nodes */
  /**
   * \brief Собрать поддерево условия проверки равенства значений
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  wns::node_ptr RawData(const std::string& raw) const {
    return wns::node_raw(raw);
  }
  /**
   * \brief Собрать поддерево условия проверки равенства значений
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Eq(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_eq);
  }
  /**
   * \brief Собрать поддерево условия проверки равенства значений
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   *
   * ??? не тоже самое что eq???
   * */
  template <class Tval>
  wns::node_ptr Is(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_is);
  }
  /**
   * \brief Собрать поддерево условия проверки неравенства значений
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Ne(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_ne);
  }
  /**
   * \brief Собрать поддерево условия проверки условия `больше или равно`
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Ge(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_ge);
  }
  /**
   * \brief Собрать поддерево условия проверки условия `строго больше`
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Gt(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_gt);
  }
  /**
   * \brief Собрать поддерево условия проверки условия `меньше или равно`
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Le(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_le);
  }
  /**
   * \brief Собрать поддерево условия проверки условия `строго меньше`
   * \tparam t Идентификатор таблицы
   * \tparam tVal Тип значения
   *
   * \param field_id Идентификатор поля
   * \param val Сравниваемое значение
   *
   * \return Узел дерева запросов
   * */
  template <class Tval>
  wns::node_ptr Lt(db_variable_id field_id, const Tval& val) const {
    return node_bind(field_id, val, wns::node_lt);
  }
  /**
   * \brief Шаблон функции собирающей узлы `Like` операций для where
   *   условий.
   *
   * \tparam t Тип таблицы
   * \tparam Tval Тип данных
   *
   * \param field_id Идентификатор обновляемого поля
   * \param val Значение
   *
   * \note Функция должна быть доступна только для символьных полей и значений
   * */
  template <class Tval>
  wns::node_ptr Like(db_variable_id field_id,
                     const char* val,
                     bool inverse = false) const;
  /**
   * \brief Шаблон функции собирающей узлы `Like` операций для where
   *   условий.
   *
   * \tparam t Тип таблицы
   *
   * \param field_id Идентификатор обновляемого поля
   * \param val Значение
   * \param inverse Искать все не похожие значения
   *
   * \note Функция должна быть доступна только для символьных полей и значений
   * */
  wns::node_ptr Like(db_variable_id field_id,
                     const std::string& val,
                     bool inverse = false) const {
    return Like(field_id, val.c_str(), inverse);
  }
  /**
   * \brief Шаблон функции собирающей узлы `Between` операций для where
   *   условий.
   *
   * \tparam t Тип таблицы
   * \tparam Tval Тип данных
   *
   * \param field_id Идентификатор обновляемого поля
   * \param min Нижняя граница значения параметров
   * \param max Верхняя граница изменения параметра
   * \param inverse Искать вне границ заданого участка
   * */
  template <class Tval>
  wns::node_ptr Between(db_variable_id field_id,
                        const Tval& min,
                        const Tval& max,
                        bool inverse = false) const;

  /* merge nodes */
  /**
   * \brief Функция связывания поддеревьев left и right узлом `AND`
   * \param left Поддерево where запроса слева
   * \param right Поддерево where запроса справа
   *
   * \return Узел корнем которого является оператор `AND`,
   *   левым подузлом(left child) узел left, правым - right
   * */
  wns::node_ptr And(const wns::node_ptr& left,
                    const wns::node_ptr& right) const {
    return wns::node_and(left, right);
  }
  /**
   * \brief Функция связывания поддеревьев n1, n2 и т.д. узлами `AND`
   *
   * \tparam Tand Типы данных которые возможно подвязать к узлам типа node_ptr
   *
   * \param n1 Первое поддерево where запроса
   * \param n2 Второе поддерево where запроса
   * \param _and Остальные поддеревья where запроса
   *
   * \return Дерево из узлов n1, n2 ... соединённых операторами `AND`
   * */
  template <class... Tand>
  wns::node_ptr And(const wns::node_ptr& n1,
                    const wns::node_ptr& n2,
                    Tand... _and) const {
    return wns::node_and(n1, n2, _and...);
  }
  /**
   * \brief Функция связывания поддеревьев left и right узлом `OR`
   * \param left Поддерево where запроса слева
   * \param right Поддерево where запроса справа
   *
   * \return Узел корнем которого является оператор `OR`,
   *   левым подузлом(left child) узел left, правым - right
   * */
  wns::node_ptr Or(const wns::node_ptr& left,
                   const wns::node_ptr& right) const {
    return wns::node_or(left, right);
  }
  /**
   * \brief Функция связывания поддеревьев n1, n2 и т.д. узлами `OR`
   *
   * \tparam Tand Типы данных которые возможно подвязать к узлам типа node_ptr
   *
   * \param n1 Первое поддерево where запроса
   * \param n2 Второе поддерево where запроса
   * \param _or Остальные поддеревья where запроса
   *
   * \return Дерево из узлов n1, n2 ... соединённых операторами `OR`
   * */
  template <class... Tor>
  wns::node_ptr Or(const wns::node_ptr& n1,
                   const wns::node_ptr& n2,
                   Tor... _or) const {
    return wns::node_or(n1, n2, _or...);
  }

 private:
  /**
   * \brief Функция создания двухпараметрических узлов - кроме `like`,
   *   `between`, `in`, без инвертирования
   * */
  template <class Tval>
  wns::node_ptr node_bind(
      db_variable_id field_id,
      const Tval& val,
      std::function<wns::node_ptr(const db_variable&, const std::string&)>
          node_create) const;

 private:
  /**
   * \brief Указатель на реализацию интерфейса таблиц
   *
   * Пространство таблиц, контекст так сказать, в котором обрабатываются
   * условия
   * */
  IDBTables* tables_;
};

/* templates */
template <db_table table>
template <class Tval>
wns::node_ptr WhereTreeSetup<table>::Like(db_variable_id field_id,
                                          const char* val,
                                          bool inverse) const {
  wns::node_ptr like = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<table>(field_id);
      like = wns::node_like(field, field2str().translate(val, field.type),
                            inverse);
    }
  } catch (idbtables_exception<table>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return like;
}

template <db_table table>
template <class Tval>
wns::node_ptr WhereTreeSetup<table>::Between(db_variable_id field_id,
                                             const Tval& min,
                                             const Tval& max,
                                             bool inverse) const {
  wns::node_ptr between = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<table>(field_id);
      between =
          wns::node_between(field, field2str().translate(min, field.type),
                            field2str().translate(max, field.type), inverse);
    }
  } catch (idbtables_exception<table>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return between;
}
template <db_table table>
template <class Tval>
wns::node_ptr WhereTreeSetup<table>::node_bind(
    db_variable_id field_id,
    const Tval& val,
    std::function<wns::node_ptr(const db_variable&, const std::string&)>
        node_create) const {
  wns::node_ptr node = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<table>(field_id);
      node = node_create(field, field2str().translate(val, field.type));
    }
  } catch (idbtables_exception<table>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return node;
}

/**
 * \brief Хранилище собранного дерева where clause
 *
 * В класс вынесены операции обновляющие дерево условий
 * */
template <db_table table>
class WhereTree {
 public:
  void Init(const wns::node_ptr& node) {
    clause_ = std::make_shared<DBWhereClause<where_node_data, table>>(node);
  }
  /**
   * \brief Наростить дерево другим поддеревом через `AND` узел
   * */
  void AddAnd(const wns::node_ptr& node) {
    if (clause_.get() != nullptr)
      clause_->AddCondition(db_operator_t::op_and, node);
  }
  /**
   * \brief Наростить дерево другим поддеревом, через `AND` узлы
   * */
  template <class... Tand>
  void AddAnd(wns::node_ptr& node, Tand... _and) {
    if (clause_.get() != nullptr) {
      clause_->AddCondition(db_operator_t::op_and, node);
      AddAnd(_and...);
    }
  }
  /**
   * \brief Наростить дерево другим поддеревом через `OR` узел
   * */
  void AddOr(const wns::node_ptr& node) {
    if (clause_.get() != nullptr)
      clause_->AddCondition(db_operator_t::op_or, node);
  }
  /**
   * \brief Наростить дерево другим поддеревом, через `OR` узлы
   * */
  template <class... Tor>
  void AddOr(wns::node_ptr& node, Tor... _or) {
    if (clause_.get() != nullptr) {
      clause_->AddCondition(db_operator_t::op_or, node);
      AddOr(_or...);
    }
  }
  /**
   * \brief Получить собранное дерево
   * */
  std::shared_ptr<DBWhereClause<where_node_data, table>> GetWhereTree() {
    return clause_;
  }

 private:
  std::shared_ptr<DBWhereClause<where_node_data, table>> clause_;
};
}  // namespace asp_db
#endif  // !_DATABASE__DB_WHERE_H_
