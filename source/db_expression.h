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
#include <variant>

#include <assert.h>

namespace asp_db {
/**
 * \brief Декларация типа функции конвертации данных поля таблицы БД типа `t`
 *   к строковому представлению
 * */
typedef std::function<std::string(db_variable_type, const std::string&)>
    DataFieldToStrF;
/**
 * \brief Функция конвертации данных поля таблицы БД типа `t`
 *   в строку по умолчанию.
 *
 * Составит строку вида:
 *   `f + " = " + v`, для текстовых полей `v` в кавычки возьмёт
 * */
std::string DataFieldToStr(db_variable_type t, const std::string& v);

/**
 * \brief Операторы отношений условий
 * \note чё там унарые то операторы то подвезли?
 * */
enum class db_operator_t {
  /** \brief Пустой оператор */
  op_empty = 0,
  /** \brief IS */
  op_is,
  /**
   * \brief IS NOT
   *
   * Попробуем задавать флагом инвертирования
   * */
  // op_not,
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
struct db_operator_wrapper {
  db_operator_t op;
  bool inverse;

  db_operator_wrapper(db_operator_t _op, bool _inverse = false);
};
/**
 * \brief Перегрузка функции получения строкового представления
 *   для типа операторов ДБ
 * */
std::string data2str(db_operator_wrapper op);

/**
 * \brief Структура данных узла запросов 'where clause'
 *
 *   В узле у нас получается может либо прятаться оператор,
 * либо имя поля, либо значение поля.
 * Очевидно:
 *   1. Оператор не может быть конечным узлом;
 *   2. `Значение поля`, вообще говоря, может форматироваться особым
 * образом, поэтому мы должны разделять 2 строковых представления -
 * имя столбца(поля) и его значения
 * */
struct where_node_data {
  typedef std::pair<db_variable_type, std::string> db_table_pair;
  /**
   * \brief Перечисление типов данных, хранимых в поле данных.
   *
   *   В поле данных может храниться имя поля, оператор where
   * условия или значение поля. Собственно говоря, они могут и
   * обрабатываться по разному. Вот 'значение поля' может хранить
   * данные специфичного формата.
   * */
  enum class ndata_type : uint32_t {
    undefined = 0,
    /// имя колонки таблицы
    field_name,
    /// оператор `where` условия ('EQ', 'LIKE' etc)
    db_operator,
    /// значение поля
    value
  };
 public:
  /**
   * \brief Объединение - или оператор where выражения или строковое
   *   представление значения таблицы бд(имени колонки или её значение)
   *
   * \note Вообще наверно вторым типом нужно прокидывать не строку,
   *   а db_variable_type или какого-либо вмда ссылку на него
   * */
  std::variant<db_operator_wrapper, db_table_pair> data;
  ndata_type ntype = ndata_type::undefined;

 public:
  /**
   * \brief Создать ноду с данными оператора
   * */
  where_node_data(db_operator_wrapper op);
  /**
   * \brief Создать ноду с данными значения столбца БД
   * */
  where_node_data(db_variable_type db_v, const std::string& value);
  /**
   * \brief Создать ноду с данными имени или значения столбца БД
   * */
  where_node_data(const db_table_pair &value);
  /**
   * \brief Создать ноду с данными имени столбца
   * */
  where_node_data(const std::string& fname);
  /**
   * \brief Получить строковое представление данных
   * */
  std::string GetString() const;
  /**
   * \brief Получить данные пары
   * */
  db_table_pair GetTablePair() const;
  /**
   * \brief Проверить тип хранимых данных `db_operator_t`
   *
   * \return true Для оператора бд, false для db_table_pair
   * */
  bool IsOperator() const;
  /**
   * \brief Проверить тип хранимых данных является именем поля
   *
   * \return true Для имени поля
   * */
  bool IsFieldName() const;
};
using where_ndata_type = where_node_data::ndata_type;
using where_table_pair = where_node_data::db_table_pair;

/**
 * \brief Структура описывающая дерево логических
 *   (или обычное арифмитическое) отношений
 *
 *   Т.к. не все узлы могу быть конечными, это накладывает
 * ограничения на функционал сбора деревьев:
 *   - можно добавлять только валидные поддеревья
 *   - корень дерева - оператор, если дерево не пусто
 * */
template <class T>
struct expression_node {
  /**
   * \brief Дерево условия для where_tree
   * */
  OWNER(db_where_tree);
  /**
   * \brief Структура данных узла
   * */
  T field_data;

 public:
  expression_node(const T& data) : field_data(data) {}

  void CreateLeftNode(const T& l) {
    left = std::make_shared<expression_node>(l);
  }
  void CreateRightNode(const T& r) {
    right = std::make_shared<expression_node>(r);
  }
  void AddLeftNode(const std::shared_ptr<expression_node>& l) { left = l; }
  void AddRightNode(const std::shared_ptr<expression_node>& r) { right = r; }

  /**
   * \brief Добавить ещё одно условие к существующему дереву
   *
   * \param op Операция БД, инициализирует новый корень дерева
   * \param right Указатель на правый(или левый - тут без разницы)
   *   узел дерева условий
   *
   * \return Новый указатель на корневой элемент дерева условий
   *
   * Example:
   *   Original tree `Tree`:
   *                       V
   *                      / \
   *   (A ^ B) V C ->    ^   C
   *                    / \
   *                   A   B
   *
   *  Tree.AddCondition(`V`, D);
   *                               V
   *                              / \
   *   ((A ^ B) V C) V D  ->     V   D
   *                            / \
   *                           ^   C
   *                          / \
   *                         A   B
   * */
  std::shared_ptr<expression_node> AddCondition(
      const T& op,
      const std::shared_ptr<expression_node>& right) {
    assert(0);
  }
  /**
   * \brief Проверить наличие подузлов
   *
   * \return true Если подузлы есть
   * */
  bool HaveSubnodes() const {
    return !((left.get() == nullptr) && (right.get() == nullptr));
  }
  /**
   * \brief Получить строковое представление дерева
   * \note Предварительный сетап данных для операций с СУБД
   * */
  std::string GetString(DataFieldToStrF dts = DataFieldToStr) const;

  std::shared_ptr<expression_node> GetLeft() const { return left; }

  std::shared_ptr<expression_node> GetRight() const { return right; }

 protected:
  std::shared_ptr<expression_node> left = nullptr;
  std::shared_ptr<expression_node> right = nullptr;
};
/**
 * \brief Собрать строку поддерева
 * */
template <class T>
std::string expression_node<T>::GetString(DataFieldToStrF dts) const {
  std::string l, r;
  std::string result;
  if (field_data.IsOperator() || field_data.IsFieldName()) {
    result = field_data.GetString();
  } else {
    auto p = field_data.GetTablePair();
    result = dts(p.first, p.second);
  }
  if (left.get())
    l = left->GetString(dts);
  if (right.get())
    r = right->GetString(dts);
  return l + result + r;
}
/**
 * \brief Декларация типа узлов условия для использования в `where_tree`
 * */
using db_condition_node = expression_node<where_node_data>;
/**
 * \brief Шаблончик на сетап поддеревьев
 *
 * Здесь общий шаблон вида:
 *   `x` operator `X`
 * */
template <db_operator_t op>
struct where_node_creator {
  static std::shared_ptr<db_condition_node> create(const std::string& fname,
                                                   const where_table_pair& value) {
    auto result =
        std::make_shared<db_condition_node>(db_operator_wrapper(op, false));
    result->CreateLeftNode(fname);
    result->CreateRightNode(value);
    return result;
  }
  static std::shared_ptr<db_condition_node> create(
      const std::string& fname,
      std::shared_ptr<db_condition_node>& cond) {
    auto result =
        std::make_shared<db_condition_node>(db_operator_wrapper(op, false));
    result->CreateLeftNode(fname);
    result->AddRightNode(cond);
    return result;
  }
};
// `like` or `not like`
// todo: why compiler doesn't allow remove full specification in separate file
template <>
struct where_node_creator<db_operator_t::op_like> {
  static std::shared_ptr<db_condition_node> create(const std::string& fname,
                                                   const std::string& value,
                                                   bool inverse) {
    auto result = std::make_shared<db_condition_node>(
        db_operator_wrapper(db_operator_t::op_like, inverse));
    result->CreateLeftNode(fname);
    result->CreateRightNode(value);
    return result;
  }
};
// `in` or `not in`
template <>
struct where_node_creator<db_operator_t::op_in> {
  std::shared_ptr<db_condition_node> result;
  where_node_creator(const std::string& fname,
                     const std::vector<std::string>& values,
                     bool inverse) {
    result = std::make_shared<db_condition_node>(
        db_operator_wrapper(db_operator_t::op_in, inverse));
    result->CreateLeftNode(fname);
    result->CreateRightNode(join_container(values, ',').str());
  }
};
// `between` or `not between`
template <>
struct where_node_creator<db_operator_t::op_between> {
  std::shared_ptr<db_condition_node> result;
  where_node_creator(const std::string& fname,
                     const std::string& left,
                     const std::string& right,
                     bool inverse) {
    result = std::make_shared<db_condition_node>(
        db_operator_wrapper(db_operator_t::op_between, inverse));
    result->CreateLeftNode(fname);
    // добавить `and` поддерево с граница `between`
    auto r = std::make_shared<db_condition_node>(
        db_operator_wrapper(db_operator_t::op_and, false));
    r->CreateLeftNode(left);
    r->CreateRightNode(right);
    result->AddRightNode(r);
  }
};

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
   *
   * \param dts Функция преобразования данных в строковые - для составления
   *   SQL строки. Нужна т.к. структуры могут содержать какие либо
   * специализированные поля либо по-своему их обрабатывать. pqxx, например,
   * текстовые данные обрабатывает через объект pqxx::nontransaction
   * */
  std::string GetString(DataFieldToStrF dts = DataFieldToStr) const;
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
  std::string GetString(DataFieldToStrF dts = DataFieldToStr) const;

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
