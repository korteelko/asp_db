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
 * \brief Структура описывающая дерево логических отношений
 * \note В общем, во внутренних узлах хранится операция, в конечных
 *   операнды, соответственно строка выражения собирается обходом
 *   в глубь
 * Прописывал ориантируюсь на СУБД Postgre потому что
 *   более/менее похожа стандарт
 * */
struct db_condition_node {
  /**
   * \brief Дерево условия для where_tree
   * */
  OWNER(db_where_tree);
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

  typedef std::function<std::string(db_type, const std::string &,
      const std::string &)> DataToStrF;

  /** \brief Конвертировать данные узла в строку */
  static std::string DataToStr(db_type, const std::string &f,
      const std::string &v);

public:
  ~db_condition_node();
  /** \brief Получить строковое представление дерева
    * \note Предварительный сетап данных для операций с СУБД */
  std::string GetString(DataToStrF dts = DataToStr) const;
  /** \brief Есть ли подузлы */
  bool IsOperator() const { return !is_leafnode; }

  db_condition_node(db_operator_t db_operator);
  db_condition_node(db_type t, const std::string &fname,
      const std::string &data);

protected:
  db_condition_node *left = nullptr;
  db_condition_node *rigth = nullptr;
  /**
   * \brief Структура данных узла
   * */
  struct {
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
    std::string field_value;
  } data;
  db_operator_t db_operator;
  /**
   * \brief КОНЕЧНАЯ
   * */
  bool is_leafnode = false;
  /**
   * \brief избегаем циклических ссылок для сборок строк
   * \note небольшой оверкилл наверное
   * \todo чекнуть можно ли обойтись без неё(убрать ето)
   * */
  mutable bool visited = false;
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
  db_where_tree(const db_where_tree &) = delete;
  db_where_tree(db_where_tree &&) = delete;
  db_where_tree &operator=(const db_where_tree &) = delete;
  db_where_tree &operator=(db_where_tree &&) = delete;

  // db_condition_tree *GetTree() const {return root_;}
  /**
   * \brief Собрать строку условного выражения
   * */
  std::string GetString(db_condition_node::DataToStrF dts =
       db_condition_node::DataToStr) const;
  /**
   * \brief Условно(не упорядочены), первая нода коллекции дерева
   * */
  std::vector<std::shared_ptr<db_condition_node>>::iterator TreeBegin();
  /**
   * \brief Условно(не упорядочены), конечная нода коллекции дерева
   * */
  std::vector<std::shared_ptr<db_condition_node>>::iterator TreeEnd();

protected:
  /**
   * \brief Собрать дерево условий по вектору узлов условий source_
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
