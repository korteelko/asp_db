/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "asp_db/db_expression.h"

#include "asp_utils/Logging.h"

#include <array>
#include <future>

namespace asp_db {
std::string DataFieldToStr(db_variable_type t, const std::string& v) {
  return (t == db_variable_type::type_char_array
          || t == db_variable_type::type_text)
             ? "'" + v + "'"
             : v;
}

db_operator_wrapper::db_operator_wrapper(db_operator_t _op, bool _inverse)
    : op(_op), inverse(_inverse) {}

std::string data2str(db_operator_wrapper op) {
  // ?? ну не знаю
  std::string result = (op.inverse) ? " NOT" : "";
  switch (op.op) {
    case db_operator_t::op_is:
      result = " IS " + result;
      break;
    /*case db_operator_t::op_not:
      result = " IS NOT ";
      break;*/
    /*case db_operator_t::op_in:
      result = result + " IN ";
      break;*/
    case db_operator_t::op_like:
      result = result + " LIKE ";
      break;
    case db_operator_t::op_between:
      result = result + " BETWEEN ";
      break;
    case db_operator_t::op_and:
      result = " AND ";
      break;
    case db_operator_t::op_or:
      result = " OR ";
      break;
    case db_operator_t::op_eq:
      result = " = ";
      break;
    case db_operator_t::op_ne:
      result = " != ";
      break;
    case db_operator_t::op_ge:
      result = " >= ";
      break;
    case db_operator_t::op_gt:
      result = " > ";
      break;
    case db_operator_t::op_le:
      result = " <= ";
      break;
    case db_operator_t::op_lt:
      result = " < ";
      break;
    case db_operator_t::op_empty:
      break;
  }
  return result;
}

where_node_data::where_node_data(db_operator_wrapper op)
    : data(op), ntype(ndata_type::db_operator) {}

where_node_data::where_node_data(db_variable_type db_v,
                                 const std::string& value)
    : data(db_table_pair(db_v, value)), ntype(ndata_type::value) {}

where_node_data::where_node_data(const db_table_pair& value) : data(value) {
  if (value.first == db_variable_type::type_empty) {
    // если тип данных в паре пуст, то строка пары - имя поля
    ntype = ndata_type::field_name;
  } else {
    // если тип данных специализирован, то это значеник поля
    ntype = ndata_type::value;
  }
}

where_node_data::where_node_data(where_node_data::ndata_type t,
                                 const db_table_pair& p)
    : data(p), ntype(t) {}

where_node_data where_node_data::field_name_node(const std::string& fname) {
  return where_node_data(ndata_type::field_name,
                         db_table_pair(db_variable_type::type_empty, fname));
}

where_node_data where_node_data::raw_data_node(const std::string& data) {
  return where_node_data(ndata_type::raw,
                         db_table_pair(db_variable_type::type_empty, data));
}

std::string where_node_data::GetString() const {
  if (IsOperator()) {
    try {
      return data2str(std::get<db_operator_wrapper>(data));
    } catch (std::bad_variant_access&) {
      // это не рантайм ошибка, это ошибка программиста
      Logging::Append(io_loglvl::info_logs,
                      "Ошибка приведения типа для "
                      "узла условий where. line"
                          + STRING_DEBUG_INFO);
    }
  }
  return GetTablePair().second;
}

where_table_pair where_node_data::GetTablePair() const {
  try {
    return std::get<db_table_pair>(data);
  } catch (std::bad_variant_access&) {
    Logging::Append(io_loglvl::info_logs,
                    "Ошибка приведения типа для узла условий where. "
                    "Функция GetTablePair вернёт NullObject\n"
                        + STRING_DEBUG_INFO);
  }
  return db_table_pair(db_variable_type::type_empty, "");
}

db_operator_wrapper where_node_data::GetOperatorWrapper() const {
  try {
    return std::get<db_operator_wrapper>(data);
  } catch (std::bad_variant_access&) {
    Logging::Append(io_loglvl::info_logs,
                    "Ошибка приведения типа для узла условий where. "
                    "Функция GetOperatorWrapper вернёт NullObject\n"
                        + STRING_DEBUG_INFO);
  }
  return db_operator_wrapper(db_operator_t::op_empty);
}

bool where_node_data::IsOperator() const {
  return ntype == ndata_type::db_operator;
}

bool where_node_data::IsFieldName() const {
  return ntype == ndata_type::field_name;
}

bool where_node_data::IsValue() const {
  return ntype == ndata_type::value;
}

bool where_node_data::IsRawData() const {
  return ntype == ndata_type::raw;
}

/* expression_node */
template <>
std::string expression_node<where_node_data>::GetString(
    DataFieldToStrF dts) const {
  std::string l, r;
  std::string result;
  bool braced = false;
  if (field_data.IsFieldName()) {
    // данные - имя поля
    result = field_data.GetString();
  } else if (field_data.IsRawData()) {
    // данные - сформатированны
    result = field_data.GetString();
  } else if (field_data.IsOperator()) {
    // данные - оператор
    result = field_data.GetString();
    braced = true;
    if (!parent) {
      // рут ноду не нужно обрамлять скобками
      braced = false;
    } else {
      auto parent_op = parent->field_data.GetOperatorWrapper();
      // for beetween - чтобы не оборачивать границы опреатора,
      //   которые разделеный `and`
      // ООООООООООООО,  сюда и про `in` запихать
      if (parent_op.op == db_operator_t::op_between)
        // в `beetween -> x and y` скобками обрамлять `x and y` не надо
        braced = false;
    }
  } else if (field_data.IsValue()) {
    // данные - значение
    auto p = field_data.GetTablePair();
    result = dts(p.first, p.second);
  } else {
    throw db_variable_exception(
        "Не обрабатываемый тип данных для where_node_data");
  }
  std::array<std::future<std::string>, 2> get_str = {
      std::async(std::launch::async,
                 [&]() {
                   return (left.get() != nullptr) ? left->GetString(dts)
                                                  : std::string();
                 }),
      std::async(std::launch::async, [&]() {
        return (right.get() != nullptr) ? right->GetString(dts) : std::string();
      })};
  std::string ans = get_str[0].get() + result + get_str[1].get();
  if (braced)
    ans = "(" + ans + ")";
  return ans;
}
/**
 * \brief Макрос регистрирующий функцию инициализации узлов дерева запросов
 *
 * Шаблоные функции `where_node_creator` портят читаемость кода, и пусть они
 * останутся на уровень абстракции пониже
 * */
#define leaf_node_access(op)                          \
  where_node_creator<db_operator_t::op_##op>::create( \
      var.fname, where_table_pair(var.type, value));
/**
 * \brief Макрос регистрирующий функцию инициализацию внутреннего узла
 *   дерева запросов
 * */
#define inner_node_access(op)                     \
  expression_node<where_node_data>::AddCondition( \
      where_node_data(db_operator_t::op_##op), left, right);

namespace where_nodes {
node_ptr node_raw(const std::string& raw_data) {
  return std::make_shared<expression_node<where_node_data>>(
      nullptr, where_node_data::raw_data_node(raw_data));
}

node_ptr node_eq(const db_variable& var, const std::string& value) {
  return leaf_node_access(eq);
}

node_ptr node_is(const db_variable& var, const std::string& value) {
  return leaf_node_access(is);
}

node_ptr node_ne(const db_variable& var, const std::string& value) {
  return leaf_node_access(ne);
}

node_ptr node_ge(const db_variable& var, const std::string& value) {
  return leaf_node_access(ge);
}

node_ptr node_gt(const db_variable& var, const std::string& value) {
  return leaf_node_access(gt);
}

node_ptr node_le(const db_variable& var, const std::string& value) {
  return leaf_node_access(le);
}

node_ptr node_lt(const db_variable& var, const std::string& value) {
  return leaf_node_access(lt);
}

node_ptr node_like(const db_variable& var,
                   const std::string& value,
                   bool inverse) {
  return where_node_creator<db_operator_t::op_like>::create(
      var.fname, where_table_pair(var.type, value), inverse);
}

node_ptr node_between(const db_variable& var,
                      const std::string& min,
                      const std::string& max,
                      bool inverse) {
  return where_node_creator<db_operator_t::op_between>::create(
      var.fname, where_table_pair(var.type, min),
      where_table_pair(var.type, max), inverse);
}

node_ptr node_and(const node_ptr& left, const node_ptr& right) {
  return inner_node_access(and);
}

node_ptr node_or(const node_ptr& left, const node_ptr& right) {
  return inner_node_access(or);
}

}  // namespace where_nodes
}  // namespace asp_db
