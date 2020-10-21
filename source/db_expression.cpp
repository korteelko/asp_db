/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_expression.h"
#include "Logging.h"

#include <stack>

namespace asp_db {
std::string DataFieldToStr(db_variable_type t, const std::string& v) {
  return (t == db_variable_type::type_char_array ||
          t == db_variable_type::type_text)
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
    case db_operator_t::op_in:
      result = result + " IN ";
      break;
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

where_node_data::where_node_data(const std::string& fname)
    : data(db_table_pair(db_variable_type::type_empty, fname)),
      ntype(ndata_type::field_name) {}

std::string where_node_data::GetString() const {
  if (IsOperator()) {
    try {
      return data2str(std::get<db_operator_wrapper>(data));
    } catch (std::bad_variant_access&) {
      // это не рантайм ошибка, это ошибка программиста
      Logging::Append(io_loglvl::info_logs,
                      "Ошибка приведения типа для "
                      "узла условий where. line" +
                          STRING_DEBUG_INFO);
    }
  }
  return GetTablePair().second;
}

where_table_pair where_node_data::GetTablePair() const {
  try {
    return std::get<db_table_pair>(data);
  } catch (std::bad_variant_access&) {
    Logging::Append(io_loglvl::info_logs,
                    "Ошибка приведения типа для "
                    "узла условий where. Функция вернёт NullObject\n" +
                        STRING_DEBUG_INFO);
  }
  return db_table_pair(db_variable_type::type_empty, "");
}

bool where_node_data::IsOperator() const {
  return ntype == ndata_type::db_operator;
}

bool where_node_data::IsFieldName() const {
  return ntype == ndata_type::field_name;
}

//#ifdef TO_REMOVE
/* db_where_tree */
db_where_tree::db_where_tree(std::shared_ptr<condition_source> source)
    : source_(source), root_(nullptr) {
  if (source_.get() != nullptr) {
    if (source_->data.size() == 1) {
      // только одно условие выборки
      root_ = source_->data[0].get();
    } else if (source_->data.size() > 1) {
      std::generate_n(
          std::back_insert_iterator<
              std::vector<std::shared_ptr<expression_node<where_node_data>>>>(
              source_->data),
          source_->data.size() - 1, []() {
            return std::make_shared<expression_node<where_node_data>>(
                db_operator_wrapper(db_operator_t::op_and, false));
          });
      construct();
    }
  }
}

std::string db_where_tree::GetString(DataFieldToStrF dts) const {
  /*if (root_) {
    for_each(source_->data.begin(), source_->data.end(),
             [](auto c) { c->visited = false; });
    return root_->GetString(dts);
  }*/
  return "";
}

void db_where_tree::construct() {
  /*
  std::stack<std::shared_ptr<db_condition_node>> st;
  for (auto nd = source_->data.begin(); nd != source_->data.end(); ++nd) {
    if ((*nd)->IsOperator()) {
      auto top1 = st.top();
      st.pop();
      auto top2 = st.top();
      st.pop();
      (*nd)->rigth = top1.get();
      (*nd)->left = top2.get();
      st.push(*nd);
    } else {
      st.push(*nd);
    }
  }
  root_ = st.top().get();
  status_ = STATUS_OK;*/
}
//#endif  // TO_REMOVE

}  // namespace asp_db
