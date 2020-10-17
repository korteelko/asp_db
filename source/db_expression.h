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
#include "ErrorWrap.h"
#include "db_defines.h"

#include <functional>
#include <memory>

#include <assert.h>

namespace asp_db {
/**
 * \brief Прототип шблона функции конвертации данных узла в строку по умолчанию.
 * */
template <class T>
std::string DataToStr(db_type t, const std::string& f, const T& v);
/**
 * \brief Функция конвертации данных узла в строку по умолчанию.
 *   Составит строку вида:
 *   `f + " = " + v`, для текстовых полей `v` в кавычки возьмёт
 * */
template <>
std::string DataToStr<std::string>(db_type t,
                                   const std::string& f,
                                   const std::string& v);
/**
 * \brief Функция конвертации данных узла в строку для
 *   типа поля таблицы
 * */
template <>
std::string DataToStr<db_variable>(db_type t,
                                   const std::string& f,
                                   const db_variable& v);
/**
 * \brief Функция конвертации данных численного узла в строку по умолчанию.
 * */
template <
    class T,
    typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
std::string DataToStr(db_type t, const std::string& f, const T& v) {
  return DataToStr<std::string>(t, f, std::to_string(v));
}

struct where_node_data;
template <>
inline std::string DataToStr<where_node_data>(db_type t,
                                              const std::string& f,
                                              const where_node_data& v) {
  return "";
}

/**
 * \brief Операторы отношений условий
 * \note чё там унарые то операторы то подвезли?
 * */
enum class db_operator_t {
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
/**
 * \brief Перегрузка функции получения строкового представления
 *   для типа операторов ДБ
 * */
std::string data2str(db_operator_t op);
/**
 * \brief Структура данных узла запросов 'where clause'
 * */
struct where_node_data {
 public:
  /// Оператор where выражения
  db_operator_t op;
  /// Строковое представление значения
  std::string value;

 public:
  where_node_data(db_operator_t _op);
  where_node_data(db_type, const std::string&, const std::string& _value);
};
/**
 * \brief Структура описывающая дерево логических отношений
 * */
template <class T>
struct expression_node {
  /**
   * \brief Дерево условия для where_tree
   * */
  OWNER(db_where_tree);
  /**
   * \brief Декларация функции приведения шаблонного типа к
   *   строковому представлению для составления запросов
   * */
  typedef std::function<std::string(db_type, const std::string&, const T&)>
      DataToStrF;
  /**
   * \brief Структура данных узла
   * */
  T field_data;

  bool visited = false;

 public:
  expression_node(const T& data) : field_data(data) {}

  /**
   * \brief Получить строковое представление дерева
   * \note Предварительный сетап данных для операций с СУБД
   * */
  std::string GetString(DataToStrF dts = DataToStr<T>) const;
  /**
   * \brief Обновить корень дерева
   * \param op Операция БД, инициализирует новый корень дерева
   * \param right Указатель на правый(или левый - тут без разницы)
   *   узел дерева условий
   *
   * \return Новый указатель на корневой элемент дерева условий
   * */
  std::shared_ptr<expression_node> CreateNewRoot(
      const T&,
      const std::shared_ptr<expression_node>& right);

 protected:
  std::shared_ptr<expression_node> left = nullptr;
  std::shared_ptr<expression_node> right = nullptr;
};
/**
 * \brief Декларация типа узлов условия для использования в `where_tree`
 * */
using db_condition_node = expression_node<where_node_data>;

template <class T>
std::string expression_node<T>::GetString(DataToStrF dts) const {
  std::string l, r;
  std::string result;
  assert(0);
  // если поддеревьев нет, собрать строку
  // if (db_operator == db_operator_t::op_empty)
  //  return dts(data.type, data.field_name, data.field_data);
  if (left.get())
    l = left->GetString(dts);
  if (right.get())
    r = right->GetString(dts);
  return result;
}

/**
 * \brief Класс инкапсулирующий функционал оператора WHERE
 * */
template <class T>
class DBWhereClause {
 public:
  DBWhereClause();
  DBWhereClause(std::shared_ptr<expression_node<T>> root);

  /**
   * \brief Собрать строку условного выражения
   * */
  std::string GetString(db_condition_node::DataToStrF dts = DataToStr<T>) const;
  /**
   * \brief Собрать строку `where` выражения по имеющимся данным
   * */
  std::string BuildClause();

 protected:
  mstatus_t status_ = STATUS_DEFAULT;
  /**
   * \brief Корень дерева условий
   * */
  std::shared_ptr<expression_node<T>> root;
};

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
  db_where_tree(const db_where_tree&) = delete;
  db_where_tree(db_where_tree&&) = delete;
  db_where_tree& operator=(const db_where_tree&) = delete;
  db_where_tree& operator=(db_where_tree&&) = delete;

  /**
   * \brief Собрать строку условного выражения
   * */
  std::string GetString(
      db_condition_node::DataToStrF dts = DataToStr<where_node_data>) const;

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
  db_condition_node* root_ = nullptr;
  /**
   * \brief Результирующая строка собранная из дерева условий
   * */
  std::string data_;
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_EXPRESSION_H_
