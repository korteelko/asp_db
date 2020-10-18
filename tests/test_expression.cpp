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
  container_t cv1(v1);
  AppendOp a1(cv1);
  // default converter
  AppendOp a2(cv1, [](container_t<std::string>& c, const std::string& t) {
    c.v.push_back(t);
  });
  // lambda converter
  AppendOp a3(
      cv1,
      [](container_t<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      [](const std::string& s) { return s; });
  // function converter
  AppendOp a4(
      cv1,
      [](container_t<std::string>& c, const std::string& t) {
        c.v.push_back(t);
      },
      convert_str);
  // functor converter
  AppendOp a5(
      cv1,
      [](container_t<std::string>& c, const std::string& t) {
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
 * \brief Тест дефолтного дерева условий
 * */
TEST(condition_node, CheckDefault) {
  /* default AppendOp */
  std::vector<std::string> v;
  container_t cv1(v);
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
      [](container_t<std::string>& c, const std::string& t) {
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
