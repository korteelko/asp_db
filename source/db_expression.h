/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_expression *
 *   В модуле прописан функционал создания деревьев запроса
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_EXPRESSION_H_
#define _DATABASE__DB_EXPRESSION_H_

#include "Common.h"
#include "db_defines.h"
#include "ErrorWrap.h"

#include <memory>


namespace asp_db {
/**
 * \brief Прототип шблона функции конвертации данных узла в строку по умолчанию.
 * */
template<class T>
std::string DataToStr(db_type t, const std::string &f, const T &v);
/**
 * \brief Функция конвертации данных узла в строку по умолчанию.
 *   Составит строку вида:
 *   `f + " = " + v`, для текстовых полей `v` в кавычки возьмёт
 * */
template<>
std::string DataToStr<std::string>(db_type t, const std::string &f,
    const std::string &v);
/**
 * \brief Функция конвертации данных численного узла в строку по умолчанию.
 * */
template<class T, typename = typename std::enable_if<
    std::is_arithmetic<T>::value, T>::type>
std::string DataToStr(db_type t, const std::string &f, const T &v) {
  return DataToStr<std::string>(t, f, std::to_string(v));
}

/**
 * \brief Структура описывающая дерево логических отношений
 * \note В общем, во внутренних узлах хранится операция, в конечных
 *   операнды, соответственно строка выражения собирается обходом
 *   в глубь
 * Прописывал ориантируюсь на СУБД Postgre потому что
 *   более/менее похожа стандарт
 * */
template <class T>
struct condition_node {
  /**
   * \brief Дерево условия для where_tree
   * */
  OWNER(db_where_tree);
  /**
   * \brief Декларация функции приведения шаблонного типа к
   *   строковому представлению для составления запросов
   * */
  typedef std::function<std::string(db_type,
      const std::string &, const T &)> DataToStrF;
  /**
   * \brief Структура данных узла
   * */
  struct node_data {
    /**
     * \brief Тип данных в БД
     * */
    db_type type;
    /**
     * \brief Имя столбца
     * */
    std::string field_name;
    /**
     * \brief Строковое представление данных
     * */
    T field_data;
  } data;
  /**
   * \brief Операторы отношений условий
   * \note чё там унарые то операторы то подвезли?
   * */
  enum class db_operator_t;

public:
  condition_node(db_operator_t db_operator)
    : data({.type = db_type::type_empty, .field_name = ""}),
      db_operator(db_operator), is_leafnode(false) {}
  condition_node(db_type t, const std::string &fname,
      const T &data)
    : data({.type = t, .field_name = fname, .field_data = data}),
      db_operator(db_operator_t::op_empty), is_leafnode(true) {}

  /**
   * \brief Получить строковое представление дерева
   * \note Предварительный сетап данных для операций с СУБД
   * */
  std::string GetString(DataToStrF dts = DataToStr<std::string>) const;
  /**
   * \brief Есть ли подузлы
   * */
  bool IsOperator() const {
    return !is_leafnode;
  }

protected:
  condition_node *left = nullptr;
  condition_node *rigth = nullptr;
  db_operator_t db_operator;
  /**
   * \brief КОНЕЧНАЯ
   * */
  bool is_leafnode = false;
  /**
   * \brief Избегаем циклических ссылок для сборок строк
   * \note Небольшой оверкилл наверное
   * \todo Чекнуть можно ли обойтись без неё(убрать ето)
   * */
  mutable bool visited = false;
};
/**
 * \brief Декларация типа узлов условия для использования в `where_tree`
 * */
using db_condition_node = condition_node<std::string>;

template <class T>
enum class condition_node<T>::db_operator_t {
  /** \brief Пустой оператор */
  op_empty = 0,
  /** \brief IS */
  op_is,
  /** \brief IS NOT */
  op_not,
  /** \brief оператор поиска в листе */
  op_in,
  /** \brief оператор поиска в коллекции */
  op_like,
  /** \brief выборка по границам */
  op_between,
  /** \brief логическое 'И' */
  op_and,
  /** \brief логическое 'ИЛИ' */
  op_or,
  /** \brief == равно */
  op_eq,
  /** \brief != не равно */
  op_ne,
  /** \brief >= больше или равно */
  op_ge,
  /** \brief > больше */
  op_gt,
  /** \brief <= меньше или равно*/
  op_le,
  /** \brief < меньше */
  op_lt,
};

template <class T>
std::string condition_node<T>::GetString(DataToStrF dts) const {
  std::string l, r;
  std::string result;
  visited = true;
  // если поддеревьев нет, собрать строку
  if (db_operator == db_operator_t::op_empty)
    return dts(data.type, data.field_name, data.field_data);
  if (left)
    if (!left->visited)
      l = left->GetString(dts);
  if (rigth)
    if (!rigth->visited)
      r = rigth->GetString(dts);
  switch (db_operator) {
    case db_operator_t::op_is:      result = l +  " IS " + r; break;
    case db_operator_t::op_not:     result = l +  " IS NOT " + r; break;
    case db_operator_t::op_in:      result = l +  " IN " + r; break;
    case db_operator_t::op_like:    result = l +  " LIKE " + r; break;
    case db_operator_t::op_between: result = l +  " BETWEEN " + r; break;
    case db_operator_t::op_and:     result = l +  " AND " + r; break;
    case db_operator_t::op_or:      result = l +  " OR " + r; break;
    case db_operator_t::op_eq:      result = l +  " = " + r; break;
    case db_operator_t::op_ne:      result = l +  " != " + r; break;
    case db_operator_t::op_ge:      result = l +  " >= " + r; break;
    case db_operator_t::op_gt:      result = l +  " > " + r; break;
    case db_operator_t::op_le:      result = l +  " <= " + r; break;
    case db_operator_t::op_lt:      result = l +  " < " + r; break;
    case db_operator_t::op_empty:
      break;
  }
  return result;
}


/**
 * \brief Дерево where условий
 * \note В общем и целом:
 *   1) не очень оптимизировано
 *   2) строки которая хочет видеть СУБД могут отличаться от того что
 *     представлено в коде, поэтому оставим возможность их менять
 * \todo Переделать это всё
 * */
class db_where_tree {
public:
  /**
   * \brief Контейнер узлов
   * */
  struct condition_source {
    std::vector<std::shared_ptr<db_condition_node>> data;
  };

public:
  db_where_tree(std::shared_ptr<condition_source> source);

  // todo: мэйби открыть???
  db_where_tree(const db_where_tree &) = delete;
  db_where_tree(db_where_tree &&) = delete;
  db_where_tree &operator=(const db_where_tree &) = delete;
  db_where_tree &operator=(db_where_tree &&) = delete;

  /**
   * \brief Собрать строку условного выражения
   * */
  std::string GetString(db_condition_node::DataToStrF dts = DataToStr<std::string>) const;

protected:
  /**
   * \brief Собрать дерево условий по вектору узлов условий source_.data
   * \note Разбивает дерево как простое выражение с ОДНОРАНГОВЫМИ операциями,
   *   т.е. например `x AND y OR z` в каком порядке поступило, так и
   *   распарсится.
   * \todo Допустим ли такой подход для разных типов операций???
   * */
  void construct();

protected:
  mstatus_t status_ = STATUS_DEFAULT;
  /**
   * \brief Контейнер-хранилище узлов условий
   * */
  std::shared_ptr<condition_source> source_;
  /**
   * \brief Корень дерева условий
   * */
  db_condition_node *root_ = nullptr;
  /**
   * \brief Результирующая строка собранная из дерева условий
   * */
  std::string data_;
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_EXPRESSION_H_
