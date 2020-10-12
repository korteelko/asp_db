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
/* db_condition_tree */
db_condition_node::db_condition_node(db_operator_t db_operator)
  : data({db_type::type_empty, "", ""}), db_operator(db_operator),
    is_leafnode(false) {}

db_condition_node::db_condition_node(db_type t, const std::string &fname,
    const std::string &data)
  : data({t, fname, data}), db_operator(db_operator_t::op_empty), is_leafnode(true) {}

db_condition_node::~db_condition_node() {}

std::string db_condition_node::DataToStr(db_type t, const std::string &f,
    const std::string &v) {
  return (t == db_type::type_char_array || t == db_type::type_text) ?
      f + " = '" + v + "'": f + " = " + v;
}

std::string db_condition_node::GetString(DataToStrF dts) const {
  std::string l, r;
  std::string result;
  visited = true;
  // если поддеревьев нет, собрать строку
  if (db_operator == db_operator_t::op_empty)
    return dts(data.type, data.field_name, data.field_value);
    //return data.field_name + " = " + data.field_value;
  if (left)
    if (!left->visited)
      l = left->GetString(dts);
  if (rigth)
    if (!rigth->visited)
      r = rigth->GetString(dts);
  switch (db_operator) {
    case db_operator_t::op_is:
      result = l +  " IS " + r;
      break;
    case db_operator_t::op_not:
      result = l +  " IS NOT " + r;
      break;
    case db_operator_t::op_in:
      result = l +  " IN " + r;
      break;
    case db_operator_t::op_like:
      result = l +  " LIKE " + r;
      break;
    case db_operator_t::op_between:
      result = l +  " BETWEEN " + r;
      break;
    case db_operator_t::op_and:
      result = l +  " AND " + r;
      break;
    case db_operator_t::op_or:
      result = l +  " OR " + r;
      break;
    case db_operator_t::op_eq:
      result = l +  " = " + r;
      break;
    case db_operator_t::op_ne:
      result = l +  " != " + r;
      break;
    case db_operator_t::op_ge:
      result = l +  " >= " + r;
      break;
    case db_operator_t::op_gt:
      result = l +  " > " + r;
      break;
    case db_operator_t::op_le:
      result = l +  " <= " + r;
      break;
    case db_operator_t::op_lt:
      result = l +  " < " + r;
      break;
    case db_operator_t::op_empty:
      break;
    // case db_operator_t::op_eq:
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
      std::generate_n(std::back_insert_iterator<
          std::vector<std::shared_ptr<db_condition_node>>>
          (source_->data), source_->data.size() - 1,
          []() { return std::make_shared<db_condition_node>
              (db_condition_node::db_operator_t::op_and); });
      construct();
    }
  }
}

std::string db_where_tree::GetString(db_condition_node::DataToStrF dts) const {
  if (root_) {
    for_each (source_->data.begin(), source_->data.end(),
        [](auto c) { c->visited = false; });
    return root_->GetString(dts);
  }
  return "";
}

std::vector<std::shared_ptr<db_condition_node>>::iterator db_where_tree::TreeBegin() {
  return source_->data.begin();
}

std::vector<std::shared_ptr<db_condition_node>>::iterator db_where_tree::TreeEnd() {
  return source_->data.end();
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
