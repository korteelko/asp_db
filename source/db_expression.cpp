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

#include <stack>

namespace asp_db {
template <>
std::string DataToStr(db_type t, const std::string& f, const std::string& v) {
  return (t == db_type::type_char_array || t == db_type::type_text)
             ? f + " = '" + v + "'"
             : f + " = " + v;
}

template <>
std::string DataToStr<where_node_data>(db_type t,
                                       const std::string& f,
                                       const where_node_data& v) {
  assert(0);
  switch () {}
}

std::string data2str(db_operator_t op) {
  std::string result;
  switch (op) {
    case db_operator_t::op_is:
      result = " IS ";
      break;
    case db_operator_t::op_not:
      result = " IS NOT ";
      break;
    case db_operator_t::op_in:
      result = " IN ";
      break;
    case db_operator_t::op_like:
      result = " LIKE ";
      break;
    case db_operator_t::op_between:
      result = " BETWEEN ";
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
              std::vector<std::shared_ptr<db_condition_node>>>(source_->data),
          source_->data.size() - 1, []() {
            return std::make_shared<db_condition_node>(db_operator_t::op_and);
          });
      construct();
    }
  }
}

where_node_data::where_node_data(db_operator_t _op) : op(_op), value("") {}

where_node_data::where_node_data(const std::string& _value)
    : op(db_operator_t::op_empty), value(_value) {}

bool where_node_data::IsOperator() const {
  return op != db_operator_t::op_empty;
}

std::string db_where_tree::GetString(db_condition_node::DataToStrF dts) const {
  if (root_) {
    for_each(source_->data.begin(), source_->data.end(),
             [](auto c) { c->visited = false; });
    return root_->GetString(dts);
  }
  return "";
}

void db_where_tree::construct() {
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
  status_ = STATUS_OK;
}
}  // namespace asp_db
