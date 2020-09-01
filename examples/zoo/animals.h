/**
 * asp_db - db api of the project 'asp_therm'
 * ===================================================================
 * * animals *
 *   Структуры данных для примера с зоопарком
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef EXAMPLES__ANIMALS_H
#define EXAMPLES__ANIMALS_H

#include <string>
#include <utility>

#include <stdint.h>

/** \brief Поле ссылки */
#define reference(_name, _type) (std::pair<row_id, _type *> _name)

#define init_flag(_name) _name ## _flag


typedef int32_t row_id;

struct animal_t {
  std::string clade;
  enum initialized {
    init_flag(clade) = 0x01,
    init_flag(full) = 0x01
  };
};

struct animal {
  std::string name;
};

#endif  // !EXAMPLES__ANIMALS_H
