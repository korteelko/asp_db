#include "gtest/gtest.h"

#include <map>

#include "db_append_functor.h"
#include "db_expression.h"

using namespace asp_db;

/* контейнеры */
template <class T>
struct my_map {
  std::map<int, T>& source;
  my_map(std::map<int, T>& _source) : source(_source) {}
  void operator()(const T& s) {
    static int i = 0;
    source.emplace(i++, s);
  }
};

/* функции конвертации */
std::string convert_str(std::string t) {
  return t;
}
std::string convert_int(int t) {
  return std::to_string(t);
}
std::string convert_double(double t) {
  return std::to_string(t);
}
int convert_s(double t) {
  return int(t);
}
int convert_s(std::string t) {
  return std::atoi(t.c_str());
}
/* функтор */
template <class T, class S, class M>
struct convert_template {
  M op;
  convert_template(M _op) : op(_op) {}
  T operator()(const S& s) { return op(s); }
};
struct cs {
  std::string operator()(const std::string& s) { return s; }
};
struct csi {
  std::string operator()(int s) { return std::to_string(s); }
};

class AspDBExpressionTest : public ::testing::Test {
 protected:
};

/**
 * \brief Проверить что шаблоны инстанцируются
 * */
TEST(AppendOp, TemplateInitialization) {
  // значения по умолчанию
  std::vector<std::string> v1;
  vector_wrapper cv1(v1);
  AppendOp a1(cv1);
  // default converter
  AppendOp a2(cv1, [](vector_wrapper<std::string>& c, const std::string& t) {
    c.v.push_back(t);
  });
  // lambda converter
  AppendOp a3(
      cv1,
      [](vector_wrapper<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      [](const std::string& s) { return s; });
  // function converter
  AppendOp a4(
      cv1,
      [](vector_wrapper<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      convert_str);
  // functor converter
  AppendOp a5(
      cv1,
      [](vector_wrapper<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      cs());

  /* мапка */
  std::map<int, std::string> m;
  my_map mm(m);
  AppendOp<std::string, my_map> ma1(
      mm, [](my_map<std::string>& c, const std::string& t) { c(t); });
  // template
  convert_template<std::string, int, std::function<std::string(int)>> y(
      [](int i) { return std::to_string(i); });
  AppendOp a6(
      mm, [](my_map<std::string>& c, const std::string& t) { c(t); }, y);
}
/**
 * \brief Тест дефолтного AppendOp для дерева условий
 * */
TEST(condition_node, CheckDefault) {
  /* default AppendOp */
  std::vector<std::string> v;
  vector_wrapper cv1(v);
  AppendOp a1(cv1);
  std::vector<std::string> v1_src{"", " ", "io oi"};
  a1(v1_src[0]);
  a1(v1_src[1]);
  a1(v1_src[2]);
  ASSERT_EQ(v.size(), v1_src.size());
  for (size_t i = 0; i < v1_src.size(); ++i)
    EXPECT_EQ(v[i], v1_src[i]);
  v.clear();
  AppendOp a2(
      cv1,
      [](vector_wrapper<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      convert_int);
  std::vector<int> v2_src{-1, 2, 32};
  a2(v2_src[0]);
  a2(v2_src[1]);
  a2(v2_src[2]);
  for (size_t i = 0; i < v2_src.size(); ++i)
    EXPECT_EQ(std::atoi(v[i].c_str()), v2_src[i]);
}

/**
 * \brief Тестим мапку
 * */
TEST(condition_node, CheckMap) {
  std::map<int, std::string> m;
  my_map mm(m);
  AppendOp<std::string, my_map> a1(
      mm, [](my_map<std::string>& c, const std::string& t) { c(t); });
  std::vector<std::string> v1_src{"", " ", "io oi"};
  a1(v1_src[0]);
  a1(v1_src[1]);
  a1(v1_src[2]);
  ASSERT_EQ(mm.source.size(), v1_src.size());
  for (auto p : mm.source) {
    EXPECT_NE(std::find(v1_src.begin(), v1_src.end(), p.second), v1_src.end());
  }
}

/* test сбора деревьев запроса */
/**
 * \brief Тест инстанцирования основного шаблона
 * */
TEST(where_node_creator, main_template) {
  // eq
  auto res_eq =
      where_node_creator<where_node_data, db_operator_t::op_eq>::create(
          "salud",
          where_table_pair(db_variable_type::type_int, "hello world!"));
  EXPECT_EQ(res_eq->field_data.GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_eq)));
  EXPECT_EQ(res_eq->GetLeft()->field_data.GetString(), "salud");
  EXPECT_EQ(res_eq->GetRight()->field_data.GetString(), "hello world!");

  EXPECT_EQ(res_eq->GetString(),
            "salud" + data2str(db_operator_wrapper(db_operator_t::op_eq)) +
                "hello world!");

  // and
  auto res_and =
      where_node_creator<where_node_data, db_operator_t::op_and>::create(
          "salud",
          where_table_pair(db_variable_type::type_int, "hello world!"));
  EXPECT_EQ(res_and->field_data.GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_and)));
  EXPECT_EQ(res_and->GetLeft()->field_data.GetString(), "salud");
  EXPECT_EQ(res_and->GetRight()->field_data.GetString(), "hello world!");
  EXPECT_EQ(res_and->GetString(),
            "salud" + data2str(db_operator_wrapper(db_operator_t::op_and)) +
                "hello world!");

  // or with nothing
  auto res_or =
      where_node_creator<where_node_data, db_operator_t::op_and>::create(
          "", where_table_pair(db_variable_type::type_bool, ""));
  EXPECT_EQ(res_or->field_data.GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_and)));
  EXPECT_EQ(res_or->GetLeft()->field_data.GetString(), "");
  EXPECT_EQ(res_or->GetRight()->field_data.GetString(), "");
  EXPECT_EQ(res_or->GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_and)));
  // or for text
  auto res_or1 =
      where_node_creator<where_node_data, db_operator_t::op_and>::create(
          "", where_table_pair(db_variable_type::type_text, ""));
  EXPECT_EQ(res_or1->field_data.GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_and)));
  EXPECT_EQ(res_or1->GetLeft()->field_data.GetString(), "");
  EXPECT_EQ(res_or1->GetRight()->field_data.GetString(), "");
  EXPECT_EQ(res_or1->GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_and)) + "''");
}

TEST(where_node_creator, specializations) {
  // simple like
  auto res_like =
      where_node_creator<where_node_data, db_operator_t::op_like>::create(
          "salud", where_table_pair(db_variable_type::type_int, "hello world!"),
          false);
  EXPECT_EQ(res_like->field_data.GetString(),
            data2str(db_operator_wrapper(db_operator_t::op_like)));
  EXPECT_EQ(res_like->GetLeft()->field_data.GetString(), "salud");
  EXPECT_EQ(res_like->GetRight()->field_data.GetString(), "hello world!");
  EXPECT_EQ(res_like->GetString(),
            "salud" + data2str(db_operator_wrapper(db_operator_t::op_like)) +
                "hello world!");
  EXPECT_STRCASEEQ(res_like->GetString().c_str(), "salud like hello world!");

  // not like
  auto notlike = db_operator_wrapper(db_operator_t::op_like, true);
  auto res_nlike =
      where_node_creator<where_node_data, db_operator_t::op_like>::create(
          "salud",
          where_table_pair(db_variable_type::type_text, "hello world!"), true);
  EXPECT_EQ(res_nlike->field_data.GetString(), data2str(notlike));
  EXPECT_EQ(res_nlike->GetLeft()->field_data.GetString(), "salud");
  EXPECT_EQ(res_nlike->GetRight()->field_data.GetString(), "hello world!");
  EXPECT_EQ(res_nlike->GetString(),
            "salud" + data2str(notlike) + "'hello world!'");
  EXPECT_STRCASEEQ(res_nlike->GetString().c_str(),
                   "salud not like 'hello world!'");

  // todo: between tests
}
TEST(db_where, complex_condition) {
  // eq
  auto res_eq =
      where_node_creator<where_node_data, db_operator_t::op_eq>::create(
          "id", where_table_pair(db_variable_type::type_int, "12"));
  EXPECT_EQ(res_eq->GetString(),
            "id" + data2str(db_operator_wrapper(db_operator_t::op_eq)) + "12");

  // not like
  auto res_nlike =
      where_node_creator<where_node_data, db_operator_t::op_like>::create(
          "func", where_table_pair(db_variable_type::type_int, "Regex"), true);
  EXPECT_STRCASEEQ(res_nlike->GetString().c_str(), "func not like Regex");
  std::shared_ptr<expression_node<where_node_data>> root =
      expression_node<where_node_data>::AddCondition(
          db_operator_wrapper(db_operator_t::op_and, false), res_eq, res_nlike);
  std::string root_str = res_eq->GetString() +
                         data2str(db_operator_wrapper(db_operator_t::op_and)) +
                         res_nlike->GetString();
  EXPECT_STRCASEEQ(root->GetString().c_str(), root_str.c_str());
}

std::string test_dts(db_variable_type t, const std::string& v) {
  std::string res = "";
  if (t == db_variable_type::type_int) {
    auto i = std::atoi(v.c_str());
    res = std::to_string(i + 2);
  } else if (t == db_variable_type::type_empty) {
    res = "empty";
  } else if (t == db_variable_type::type_text) {
    res = "'" + v + "'";
  }
  return res;
}

TEST(db_where, checkDts) {
  auto res_eq =
      where_node_creator<where_node_data, db_operator_t::op_eq>::create(
          "ob", where_table_pair(db_variable_type::type_int, "4"));
  auto res_is =
      where_node_creator<where_node_data, db_operator_t::op_is>::create(
          "obo", where_table_pair(db_variable_type::type_text, "hello world!"));
  std::shared_ptr<expression_node<where_node_data>> root =
      expression_node<where_node_data>::AddCondition(
          db_operator_wrapper(db_operator_t::op_and, false), res_eq, res_is);
  std::string root_str = res_eq->GetString() +
                         data2str(db_operator_wrapper(db_operator_t::op_and)) +
                         res_is->GetString();
  EXPECT_STRCASEEQ(root->GetString().c_str(), root_str.c_str());
}

TEST(DBWhereClause, InitWhereClause) {
  auto w = DBWhereClause<where_node_data>::CreateRoot<db_operator_t::op_eq>(
      "fname", where_table_pair(db_variable_type::type_text, "hello world!"));
  auto res_is =
      where_node_creator<where_node_data, db_operator_t::op_lt>::create(
          "obo", where_table_pair(db_variable_type::type_int, "4"));

  w.AddCondition(db_operator_wrapper(db_operator_t::op_and, false), res_is);
  std::string wstr = "fname" + data2str(db_operator_t::op_eq) +
                     "'hello world!'" + data2str(db_operator_t::op_and) +
                     "obo" + data2str(db_operator_t::op_lt) + "4";
  EXPECT_STRCASEEQ(w.GetString().c_str(), wstr.c_str());
}
