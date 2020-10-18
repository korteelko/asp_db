/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * library_structs *
 *   Структуры данных для примера с библиотекой
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef EXAMPLES__LIBRARY_STRUCTS_H
#define EXAMPLES__LIBRARY_STRUCTS_H

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <stdint.h>

/* примерные типы данных */
typedef int32_t row_id;
/** \brief Язык, как пример enum */
enum language_t {
  lang_undef = 0,
  lang_rus = 1,
  lang_eng = 2,
  lang_esp = 3,
  lang_ita = 4
};
/** \brief Вот например книжка */
struct book {
  /** \brief Флаги инициализированных полей */
  enum initialized_flags {
    f_title = 0x01,
    f_pub_year = 0x02,
    f_lang = 0x04,
    f_id = 0x08,
    f_full = 0x0f
  };
  inline bool IsFlagSet(initialized_flags f) const { return f & initialized; }
  /** \brief Номер в таблице БД */
  row_id id;
  /** \brief Есть у неё название */
  std::string title;
  /** \brief Год публикации позволит разглядеть спрятанное */
  int first_pub_year;
  /** \brief Язык оригинала */
  language_t lang;

  int32_t initialized = 0;
};
inline std::string to_str(const book& b) {
  return b.title + ", year: " + std::to_string(b.first_pub_year) +
         ", lang code " + std::to_string(b.lang) + ", id " +
         std::to_string(b.id);
}
inline std::ostream& operator<<(std::ostream& stream, const book& b) {
  return stream << to_str(b) << std::endl;
}

/** \brief Перевод */
struct translation {
  /** \brief Флаги инициализированных полей */
  enum initialized_flags {
    f_book_p = 0x01,
    f_lang = 0x02,
    f_tr_name = 0x04,
    f_translators = 0x08,
    f_id = 0x10,
    f_full = 0x1f
  };
  inline bool IsFlagSet(initialized_flags f) const { return f & initialized; }
  row_id id;
  /** \brief Книги */
  std::pair<row_id, book*> book_p;
  language_t lang;
  std::string translated_name;
  std::string translators;

  int32_t initialized = 0;
};

/** \brief Писатель */
struct author {
  enum initialized_flags {
    f_name = 0x01,
    f_b_year = 0x02,
    f_d_year = 0x04,
    f_books = 0x08,
    f_id = 0x10,
    f_full = 0x1f
  };
  inline bool IsFlagSet(initialized_flags f) const { return f & initialized; }
  row_id id;
  /** \brief Имя */
  std::string name;
  int born_year;
  int died_year;
  /** \brief Имена книг */
  std::vector<std::string> books;

  int32_t initialized = 0;
};

template <class T>
std::string insertValue2str(const T& s) {
  return std::to_string(s);
}

#endif  // !EXAMPLES__LIBRARY_STRUCTS_H
