#ifndef EXAMPLE_META_H
#define EXAMPLE_META_H

#include <string>
#include <vector>
#include <utility>

#include <stdint.h>


#define ASP_TABLE

// c placeholders
#define field(x, ...) x
#define field_fkey(x, xt, tt, ...) std::pair<xt, tt *> x
#define primary_key(x)
#define unique(...)
#define reference(x, string)


// datatypes
typedef size_t incremental;
typedef ssize_t bigint;
typedef std::string text;
template <class T>
using container = std::vector<T> ;

#endif  // !EXAMPLE_META_H

