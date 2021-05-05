/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_expression *
 *   В модуле прописан функционал создания деревьев запроса
 * ===================================================================
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_EXPRESSION_H_
#define _DATABASE__DB_EXPRESSION_H_

#include "asp_utils/Common.h"
#include "asp_utils/ErrorWrap.h"
#include "asp_utils/Logging.h"
#include "db_defines.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
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
  /**
   * \brief оператор поиска в листе
   * \todo Добавление этого функционала вынесено в отдельную задачу. Там
   *   ещё можно SELECT запрос вместо списка аргументов подцеплять
   *
   * */
  // op_in,
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
  /**
   * \brief != не равно
   * \todo убрать его наверное, флагом инверсии задавать
   * */
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
  /**
   * \brief Оператор выражения
   * */
  db_operator_t op;
  /**
   * \brief Флаг инверсии оператора
   *
   *   Актуален для операторов `IN`, `LIKE`, `BETWEEN` и переводит их
   * соответственно в `NOT IN`, `NOT LIKE`, `NOT BETWEEN`
   *
   * \todo Наверное переименовать его, не соовсем суть отражает
   * */
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
  /* NOTE:
   * - Наверное в структуру нужно было завернуть, а не pair
   * - Имя тоже неудачное, указывает на пару, а должно на
   *   то что это обёртка над значением табицы*/
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
    value,
    /// уже сформатированные данные, только записать
    raw
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
  explicit where_node_data(db_operator_wrapper op);
  /**
   * \brief Создать ноду с данными имени или значения столбца БД
   * */
  explicit where_node_data(const db_table_pair& value);
  /**
   * \brief Создать ноду с данными значения столбца БД
   * */
  where_node_data(db_variable_type db_v, const std::string& value);
  /**
   * \brief Создать ноду с данными имени столбца
   * */
  static where_node_data field_name_node(const std::string& fname);
  /**
   * \brief Создать ноду с уже сфомированными данными
   * */
  static where_node_data raw_data_node(const std::string& data);

  /**
   * \brief Получить строковое представление данных
   * */
  std::string GetString() const;
  /**
   * \brief Получить данные пары
   * */
  db_table_pair GetTablePair() const;
  /**
   * \brief Получить объект-обёртку над оператором
   * */
  db_operator_wrapper GetOperatorWrapper() const;
  /**
   * \brief Проверить тип хранимых данных `db_operator_t`
   *
   * \return true Для оператора бд, false для db_table_pair
   * */
  bool IsOperator() const;
  /**
   * \brief Является ли тип хранимых данных именем поля
   *
   * \return true Для имени поля
   * */
  bool IsFieldName() const;
  /**
   * \brief Является ли тип хранимых данных значением
   *
   * \return true Для `value`
   * */
  bool IsValue() const;
  /**
   * \brief Является ли тип хранимых данных `форматированными данными`
   *
   * \return true Для `raw`
   * */
  bool IsRawData() const;

 protected:
  where_node_data(ndata_type t, const db_table_pair& p);
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
  expression_node(expression_node* parent, const T& data)
      : field_data(data), parent(parent) {}

  virtual ~expression_node() = default;

  void CreateLeftNode(const T& l) {
    left = std::make_shared<expression_node>(this, l);
  }
  void CreateRightNode(const T& r) {
    right = std::make_shared<expression_node>(this, r);
  }
  void AddLeftNode(const std::shared_ptr<expression_node>& l) {
    if (l.get() != nullptr) {
      left = l;
      left->parent = this;
    } else {
      left = nullptr;
    }
  }
  void AddRightNode(const std::shared_ptr<expression_node>& r) {
    if (r.get() != nullptr) {
      right = r;
      right->parent = this;
    } else {
      right = nullptr;
    }
  }
  /**
   * \brief Собрать поддерево из 2 инициализированных поддеревьев и оператора
   *
   * \param op Операция БД, инициализирует новый корень дерева
   * \param left Указатель на левый узел дерева условий
   * \param right Указатель на правый узел дерева условий
   *
   * \return Указатель на новый узел, собранный по переданным данным,
   *   узел empty оператора, если оба узла пусты, или единственный не
   *   пустой узел
   * */
  static std::shared_ptr<expression_node> AddCondition(
      const T& op,
      const std::shared_ptr<expression_node>& left,
      const std::shared_ptr<expression_node>& right) {
    std::shared_ptr<expression_node> root = nullptr;
    if ((left.get() != nullptr) && (right.get() != nullptr)) {
      // если оба узла не пусты, создадим поддерево с корнем `op`
      root = std::make_shared<expression_node>(nullptr, op);
      root->AddLeftNode(left);
      root->AddRightNode(right);
    } else if ((left.get() == nullptr) && (right.get() == nullptr)) {
      // если оба узла пусты
      root = std::make_shared<expression_node>(
          nullptr,
          where_node_data(db_operator_wrapper(db_operator_t::op_empty)));
    } else if (left.get() != nullptr) {
      // не пуст только левый узел
      root = left;
    } else {
      // не пуст только правый узел
      root = right;
    }
    return root;
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
  expression_node* parent = nullptr;
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
  if (field_data.IsFieldName() || field_data.IsOperator()) {
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
 * \brief Собрать строку поддерева
 *
 * \note Такие перегрузки разрешены?
 * */
template <>
std::string expression_node<where_node_data>::GetString(
    DataFieldToStrF dts) const;

/**
 * \brief Шаблончик на сетап поддеревьев
 *
 * Здесь общий шаблон вида:
 *   `x` operator `X`
 * \todo Найти более бодходящее имя, с учётом статичной типизации
 * */
template <db_operator_t op>
struct where_node_creator {
  /**
   * \brief Создать поддерево оператора, где в корневом узле хранится
   *   оператор, специализированный шаблоном, слева узел имени поля `fname`,
   *   справа значение поля.
   *
   * \param fname Имя поля
   * \param value Обёртка над значением поля таблицы БД, содержит
   *   и тип поля, и его строковое представление
   * */
  static std::shared_ptr<expression_node<where_node_data>> create(
      const std::string& fname,
      const where_table_pair& value) {
    auto result = std::make_shared<expression_node<where_node_data>>(
        nullptr, where_node_data(db_operator_wrapper(op, false)));
    result->CreateLeftNode(where_node_data::field_name_node(fname));
    result->CreateRightNode(where_node_data(value));
    return result;
  }
  static std::shared_ptr<expression_node<where_node_data>> create(
      const std::string& fname,
      std::shared_ptr<expression_node<where_node_data>>& cond) {
    auto result = std::make_shared<expression_node<where_node_data>>(
        nullptr, where_node_data(db_operator_wrapper(op, false)));
    result->CreateLeftNode(where_node_data::field_name_node(fname));
    result->AddRightNode(cond);
    return result;
  }
  /**
   * \brief Создать узел дерева запросов который уже содержит в себе заданную
   *   строку, которую необходимо просто добавить к остальному дереву
   * */
  static std::shared_ptr<expression_node<where_node_data>> create_raw(
      const std::string& raw_data) {
    return std::make_shared<expression_node<where_node_data>>(
        nullptr, where_node_data::raw_data_node(raw_data));
  }
};
// `like` or `not like`
template <>
struct where_node_creator<db_operator_t::op_like> {
  static std::shared_ptr<expression_node<where_node_data>> create(
      const std::string& fname,
      const where_table_pair& value,
      bool inverse) {
    auto result = std::make_shared<expression_node<where_node_data>>(
        nullptr,
        where_node_data(db_operator_wrapper(db_operator_t::op_like, inverse)));
    result->CreateLeftNode(where_node_data::field_name_node(fname));
    result->CreateRightNode(where_node_data(value));
    return result;
  }
};
// `in` or `not in`
/*
template <class T>
struct where_node_creator<T, db_operator_t::op_in> {
  static std::shared_ptr<expression_node<T>> create(
      const std::string& fname,
      db_variable_type type,
      const std::vector<std::string>& values,
      bool inverse) {
    auto result = std::make_shared<expression_node<T>>(nullptr,
        db_operator_wrapper(db_operator_t::op_in, inverse));
    result->CreateLeftNode(fname);
    auto values_str = db_variable::TranslateFromVector(values.begin(),
values.end()); result->CreateRightNode(where_table_pair(type, values_str));
    //result->CreateRightNode(
    //    where_table_pair(type, "(" + join_container(values, ',').str() +
")")); return result;
  }
};
*/
// `between` or `not between`
template <>
struct where_node_creator<db_operator_t::op_between> {
  static std::shared_ptr<expression_node<where_node_data>> create(
      const std::string& fname,
      const where_table_pair& left,
      const where_table_pair& right,
      bool inverse) {
    auto result = std::make_shared<expression_node<where_node_data>>(
        nullptr, where_node_data(
                     db_operator_wrapper(db_operator_t::op_between, inverse)));
    result->CreateLeftNode(where_node_data::field_name_node(fname));
    // добавить `and` поддерево с граница `between`
    auto r = std::make_shared<expression_node<where_node_data>>(
        nullptr,
        where_node_data(db_operator_wrapper(db_operator_t::op_and, false)));
    r->CreateLeftNode(where_node_data(left));
    r->CreateRightNode(where_node_data(right));
    result->AddRightNode(r);
    return result;
  }
};

/**
 * \brief Класс инкапсулирующий функционал сбора выражения WHERE
 *
 * \todo Заменить встречающийся далее тип `db_operator_wrapper`
 *   на шаблонный тип
 *
 * Шаблон валиден для типа where_node_data
 * */
template <class T>
class DBWhereClause {
 public:
  template <db_operator_t _op>
  static DBWhereClause CreateRoot(const std::string& fname,
                                  const where_table_pair& value) {
    auto r = where_node_creator<_op>::create(fname, value);
    return DBWhereClause(r);
  }
  DBWhereClause(std::shared_ptr<expression_node<T>> _root) : root(_root) {}
  /**
   * \brief Добавить поддерево условий привязавшись к уже имеющемуся
   *   оператором `_op`
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
  mstatus_t AddCondition(db_operator_wrapper _op,
                         const std::shared_ptr<expression_node<T>>& condition) {
    mstatus_t ret = STATUS_HAVE_ERROR;
    try {
      auto r = expression_node<T>::AddCondition(where_node_data(_op), root,
                                                condition);
      root = r;
      ret = STATUS_OK;
    } catch (db_variable_exception& e) {
      Logging::Append(io_loglvl::err_logs,
                      "Во время попытки добавления условия"
                      " в AddCondition перхвачено исключение: ");
      Logging::Append(io_loglvl::err_logs, e.what());
    } catch (std::bad_alloc&) {
      Logging::Append(io_loglvl::err_logs,
                      "Во время попытки добавления условия"
                      " в AddCondition перхвачено исключение bad_alloc");
    } catch (std::exception& e) {
      Logging::Append(io_loglvl::err_logs,
                      "Во время попытки добавления условия"
                      " в AddCondition перхвачено общее исключение:\n" +
                          std::string(e.what()));
    }
    return ret;
  }
  /**
   * \brief Влить в текущее дерево другре поддерево DBWhereClause,
   *   разбив их оператором `op`
   * */
  mstatus_t MergeWhereClause(db_operator_wrapper _op,
                             const DBWhereClause& wclause) {
    return AddCondition(_op, wclause.root);
  }
  /**
   * \brief Собрать строку условного выражения
   *
   * \param dts Функция преобразования данных в строковые - для составления
   *   SQL строки. Нужна т.к. структуры могут содержать какие либо
   * специализированные поля либо по-своему их обрабатывать. pqxx, например,
   * текстовые данные обрабатывает через объект pqxx::nontransaction
   * */
  std::string GetString(DataFieldToStrF dts = DataFieldToStr) const {
    if (root.get() != nullptr) {
      return root->GetString(dts);
    }
    return "";
  }

 protected:
  /**
   * \brief Корень дерева условий
   * */
  std::shared_ptr<expression_node<T>> root;
};

/* todo: нейминг уровня \b */
/**
 * \brief Пространство имён создания деревьев условных выражений
 *   для составления where подстрок запросов
 * */
namespace where_nodes {
typedef std::shared_ptr<expression_node<where_node_data>> node_ptr;

/* leaf nodes */
/*  simple operators */
node_ptr node_raw(const std::string& raw_data);
node_ptr node_eq(const db_variable& var, const std::string& value);
node_ptr node_is(const db_variable& var, const std::string& value);
node_ptr node_ne(const db_variable& var, const std::string& value);
node_ptr node_ge(const db_variable& var, const std::string& value);
node_ptr node_gt(const db_variable& var, const std::string& value);
node_ptr node_le(const db_variable& var, const std::string& value);
node_ptr node_lt(const db_variable& var, const std::string& value);
/*  special operators */
node_ptr node_like(const db_variable& var,
                   const std::string& value,
                   bool inverse = false);
/*node_ptr node_in(const db_variable &var,
                 const std::vector<std::string> &values,
                 bool inverse) {
  return where_node_creator<where_node_data, db_operator_t::op_in>::create(
        var.fname,
        var.type,
        values,
        inverse);
}*/
node_ptr node_between(const db_variable& var,
                      const std::string& min,
                      const std::string& max,
                      bool inverse = false);

/* inner nodes */
node_ptr node_and(const node_ptr& left, const node_ptr& right);
node_ptr node_or(const node_ptr& left, const node_ptr& right);
template <class... Tand>
node_ptr node_and(const node_ptr& left, const node_ptr& right, Tand... _and) {
  return node_and(node_and(left, right), _and...);
}
template <class... Tor>
node_ptr node_or(const node_ptr& left, const node_ptr& right, Tor... _or) {
  return node_or(node_or(left, right), _or...);
}
}  // namespace where_nodes
}  // namespace asp_db

#endif  // !_DATABASE__DB_EXPRESSION_H_
