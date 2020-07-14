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

/** \brief Перевод */
struct translation {
  row_id id;
  /** \brief Книги */
  std::pair<row_id, book *> book_p;
  language_t lang;
  std::string translated_name;
  std::string translators;

  int32_t initialized = 0;
};

/** \brief Писатель */
struct author {
  row_id id;
  /** \brief Имя */
  std::string name;
  int born_year;
  int died_year;
  /** \brief Имена книг */
  std::vector<std::string> books;

  int32_t initialized = 0;
};

#endif  // !EXAMPLES__LIBRARY_STRUCTS_H
