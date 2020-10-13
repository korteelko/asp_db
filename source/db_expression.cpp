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
template<>
std::string DataToStr(db_type t, const std::string &f,
    const std::string &v) {
  return (t == db_type::type_char_array || t == db_type::type_text) ?
      f + " = '" + v + "'": f + " = " + v;
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
