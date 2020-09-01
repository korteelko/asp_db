#ifndef EXAMPLE_TABLES_H
#define EXAMPLE_TABLES_H

#include "meta.h"

#include <numeric>
#include <iostream>


/**
 * \brief
 *   Пример сруктуры данных, аналогичной таблице в БД
 *   с именем 'example1_info'.
 * */
struct ASP_TABLE example1_info  {
  field(incremental id, NOT_NULL);
  field(text name, NOT_NULL);

  /* параметры таблицы */
  primary_key(id)
};
std::ostream &operator<<(std::ostream &str, const example1_info &e) {
  return str << e.id << ' ' << e.name << std::endl;
}

/**
 * \brief
 *   Пример сруктуры данных, аналогичной таблице в БД
 *   с именем 'example2_info', хранит ссылку на example1_info.
 * */
struct ASP_TABLE example2_info  {
  field(incremental id, NOT_NULL);
  field(text name, NOT_NULL);
  field(container<text> strings, ARRAY);
  field(int num);
  /* pair<bigint id, another_table *> --
   *   reference to table 'example_info1' with id = pair.first,
   *     (then pair.second == nullptr).
   *   or reference to table 'example_info1' by pointer - pair.second != nullptr,
   *     (then pair.first = -1).
   *   or pair.first > -1, pair.second != nullpt.
   *     But pair.second->id must be equal pair.first
   **/
  field_fkey(fkey, bigint, example1_info, NOT_NULL);

  /* параметры таблицы */
  primary_key(id)
  unique(name, num)
  reference(fkey, "id in example1_info onupdate cascade ondelete cascade")
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

