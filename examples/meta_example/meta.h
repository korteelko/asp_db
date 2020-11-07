#ifndef EXAMPLE_META_H
#define EXAMPLE_META_H

#include <string>
#include <vector>
#include <utility>

#include <stdint.h>


template <class T>
std::string field2str_(const T&);

// todo: maybe variadic macroses or templates
#define field2str_set(name, f) \
  template <class T> \
  std::string field2str_ ## name(const T &t) { \
    return f(t);\
  }
#define str2field_set(name, f) \
  template <class T> \
  void str2field_ ## name(const std::string &s, T *t) { \
    f(s, t);\
  }

#define ASP_TABLE

// c placeholders
#define field(t, x, ...) t x
/* прикинемся парой */
#define field_fkey(t, x, ...) std::pair<t, void *> x
/* функции конвертации строковых представлений */
#define str_functions(field, to_str_f, from_str_f) \
  field2str_set(field, to_str_f) \
  str2field_set(field, from_str_f)
//#define str_functions(field, to_str_f, from_str_f)

#define primary_key(x)
#define unique_complex(...)
#define reference(x, string, ...)


// datatypes
typedef size_t incremental;
typedef ssize_t bigint;
typedef std::string text;

template <class T>
using container = std::vector<T>;

#endif  // !EXAMPLE_META_H

