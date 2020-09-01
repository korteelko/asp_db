#ifndef EXAMPLE_TABLES_H
#define EXAMPLE_TABLES_H

#include "meta.h"

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

struct ASP_TABLE example2_info  {
  field(incremental id, NOT_NULL);
  field(text name, NOT_NULL);
  field(container<text> strings, ARRAY);
  field(int num);
  field_fkey(fkey, bigint, example2_info, NOT_NULL);

  /* параметры таблицы */
  primary_key(id)
  unique(name, num)
  reference(fkey, "id in example1_info onupdate cascade ondelete cascade")
};

#endif  // !EXAMPLE_TABLES_H

