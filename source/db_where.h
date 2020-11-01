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
/**
 * \brief Класс сборки дерева условий where запроса
 * */
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
  template <db_table t, class Tval>
  wns::node_ptr Eq(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_eq);
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
  template <db_table t, class Tval>
  wns::node_ptr Is(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_is);
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
  template <db_table t, class Tval>
  wns::node_ptr Ne(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_ne);
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
  template <db_table t, class Tval>
  wns::node_ptr Ge(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_ge);
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
  template <db_table t, class Tval>
  wns::node_ptr Gt(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_gt);
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
  template <db_table t, class Tval>
  wns::node_ptr Le(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_le);
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
  template <db_table t, class Tval>
  wns::node_ptr Lt(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_lt);
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
  template <db_table t, class Tval>
  wns::node_ptr Like(db_variable_id field_id,
                     const char* val,
                     bool inverse = false);
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
  template <db_table t>
  wns::node_ptr Like(db_variable_id field_id,
                     const std::string& val,
                     bool inverse = false) {
    return Like<t>(field_id, val.c_str(), inverse);
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
  template <db_table t, class Tval>
  wns::node_ptr Between(db_variable_id field_id,
                        const Tval& min,
                        const Tval& max,
                        bool inverse = false);

  /* merge nodes */
  /**
   * \brief Функция связывания поддеревьев left и right узлом `AND`
   * \param left Поддерево where запроса слева
   * \param right Поддерево where запроса справа
   *
   * \return Узел корнем которого является оператор `AND`,
   *   левым подузлом(left child) узел left, правым - right
   * */
  inline wns::node_ptr And(const wns::node_ptr& left,
                           const wns::node_ptr& right) {
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
                    Tand... _and) {
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
  inline wns::node_ptr Or(const wns::node_ptr& left,
                          const wns::node_ptr& right) {
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
                   Tor... _or) {
    return wns::node_or(n1, n2, _or...);
  }

 private:
  /**
   * \brief Функция создания двухпараметрических узлов - кроме `like`,
   *   `between`, `in`, без инвертирования
   * */
  template <db_table t, class Tval>
  wns::node_ptr node_bind(
      db_variable_id field_id,
      const Tval& val,
      std::function<wns::node_ptr(const db_variable&, const std::string&)>
          node_create);

 private:
  /**
   * \brief
   *
   * Пространство таблиц, контекст так сказать, в котором обрабатываются
   * условия
   * */
  IDBTables* tables_;
};

/* templates */
template <db_table t, class Tval>
wns::node_ptr WhereTreeSetup::Like(db_variable_id field_id,
                                   const char* val,
                                   bool inverse) {
  wns::node_ptr like = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<t>(field_id);
      like = wns::node_like(field, field2str().translate(val, field.type),
                            inverse);
    }
  } catch (idbtables_exception<t>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return like;
}

template <db_table t, class Tval>
wns::node_ptr WhereTreeSetup::Between(db_variable_id field_id,
                                      const Tval& min,
                                      const Tval& max,
                                      bool inverse) {
  wns::node_ptr between = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<t>(field_id);
      between =
          wns::node_between(field, field2str().translate(min, field.type),
                            field2str().translate(max, field.type), inverse);
    }
  } catch (idbtables_exception<t>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return between;
}
template <db_table t, class Tval>
wns::node_ptr WhereTreeSetup::node_bind(
    db_variable_id field_id,
    const Tval& val,
    std::function<wns::node_ptr(const db_variable&, const std::string&)>
        node_create) {
  wns::node_ptr node = nullptr;
  try {
    if (tables_) {
      const db_variable& field = tables_->GetFieldById<t>(field_id);
      node = node_create(field, field2str().translate(val, field.type));
    }
  } catch (idbtables_exception<t>& e) {
    // добавить к сообщению об ошибке дополнителоьную информацию
    Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
  }
  return node;
}

}  // namespace asp_db
#endif  // !_DATABASE__DB_WHERE_H_
