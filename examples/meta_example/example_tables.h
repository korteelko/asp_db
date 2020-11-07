#ifndef EXAMPLE_TABLES_H
#define EXAMPLE_TABLES_H

#include "meta.h"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <utility>


/**
 * \brief
 *   Пример сруктуры данных, аналогичной таблице в БД
 *   с именем 'example1_info'.
 * */
struct ASP_TABLE example1_info  {
  field(incremental, id, NOT_NULL);
  field(text, name, NOT_NULL);

  /* параметры таблицы */
  primary_key(id)
};
std::ostream &operator<<(std::ostream &str, const example1_info &e) {
  return str << e.id << ' ' << e.name << std::endl;
}

/* преобразовать example2_info.name к строковому представлению,
 *   для примера, запишем в верхнем регистре */
inline std::string name_to_str(const text &name) {
  std::string res;
  std::transform(name.begin(), name.end(), res.begin(), ::toupper);
  return res;
}
inline void str_to_name(const text &orig, std::string *s) {
  *s = "";
  std::transform(orig.begin(), orig.end(), s->begin(), ::tolower);
}
/**
 * \brief Пример сруктуры данных, аналогичной таблице в БД
 *   с именем 'example2_info', хранит ссылку на example1_info.
 * */
struct ASP_TABLE example2_info  {
  /* todo может id неявным сделать? */
  field(incremental, id, NOT_NULL);

  /* name setup */
  field(text, name, NOT_NULL);
  /* generate functions */
  //   std::string field2str_name(const std::string &t);
  //   void str2field_name(const std::string &s, std::string *t);
  str_functions(name, name_to_str, str_to_name)

  /* strings */
  field(container<text>, strings, ARRAY);

  /* num */
  field(int, num);
  /* pair<bigint id, another_table *> --
   *   reference to table 'example_info1' with id = pair.first,
   *     (then pair.second == nullptr).
   *   or reference to table 'example_info1' by pointer - pair.second != nullptr,
   *     (then pair.first = -1).
   *   or pair.first > -1, pair.second != nullpt.
   *     But pair.second->id must be equal pair.first
   **/
  field_fkey(bigint, fkey, NOT_NULL);

  /* функции не тривиальной конвертации поля структуры к строковому
   *   представлению, добавляемому в SQL запросам */
  // str_functions(name,)

  /* параметры таблицы */
  primary_key(id)
  unique_complex(name, num)
  // reference(fkey, "id in example1_info onupdate cascade ondelete cascade")
  /*       |field|ftable(fkey)     |on update|on delete */
  reference(fkey, example1_info(id), CASCADE, CASCADE)
};
std::ostream &operator<<(std::ostream &str, const example2_info &e) {
  std::string s;
  if (e.strings.size()) {
    s = std::accumulate(std::next(e.strings.begin()), e.strings.end(),
        e.strings[0], [](std::string &line, const std::string &val) {
        return line += ", " + val;});
  }
  return str << e.id << ' ' << e.name << ' ' << s << std::endl;
}

#endif  // !EXAMPLE_TABLES_H

