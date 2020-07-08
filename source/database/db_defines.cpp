/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "db_connection_manager.h"
#include "db_defines.h"
#include "Logging.h"

#include <map>

#include <assert.h>
#include <string.h>


namespace asp_db {
std::string db_client_to_string(db_client client) {
  std::string name = "";
  switch (client) {
    case db_client::NOONE:
      name = "dummy connection";
      break;
    case db_client::POSTGRESQL:
      name = "postgresql";
      break;
    default:
      throw DBException("Неизвестный клиент БД");
      name = "undefined";
  }
  return name;
}

/* db_variable */
db_variable::db_variable(db_variable_id fid, const char *fname,
    db_var_type type, db_variable_flags flags, int len)
  : fid(fid), fname(fname), type(type), flags(flags), len(len) {}

merror_t db_variable::CheckYourself() const {
  merror_t ew = ERROR_SUCCESS_T;
  if (fname == NULL) {
    ew = ERROR_DB_VARIABLE;
    Logging::Append(io_loglvl::err_logs, "пустое имя поля таблицы бд" +
        STRING_DEBUG_INFO);
  } else if (type == db_var_type::type_char_array &&
      (!flags.is_array || len < 1)) {
    ew = ERROR_DB_VARIABLE;
    Logging::Append(io_loglvl::err_logs,
        "установки поля char array не соответствуют ожиданиям" +
        STRING_DEBUG_INFO);
  } else if (flags.is_array && flags.is_reference) {
    ew = ERROR_DB_VARIABLE;
    Logging::Append(io_loglvl::err_logs,
        "недопустимая опция для массива - ссылка на внешнюю таблицу");
  }
  return ew;
}

std::string db_variable::TranslateFromVector(
    const std::vector<std::string> &vec) {
  std::string result = "";
  for (const auto &x: vec)
    result += std::to_string(x.size()) + " " + x ;
  return result;
}

mstatus_t db_variable::TranslateToVector(const std::string &str,
    std::vector<std::string> *vec_p) {
  vec_p->clear();
  const char *ptr = str.c_str();
  size_t pos = 0;
  size_t start = 0;
  mstatus_t st = STATUS_OK;
  while (start < str.size()) {
    int n = std::atoi(ptr + start);
    pos = str.find(' ', start);
    if (pos != std::string::npos) {
      if (n > 0) {
        if (pos + n < str.size()) {
          vec_p->push_back(str.substr(pos + 1, n));
          pos = pos + n;
        } else {
          st = STATUS_HAVE_ERROR;
          break;
        }
      } else {
        vec_p->push_back("");
      }
    } else {
      break;
    }
    start = pos + 1;
  }
  if (vec_p->size())
    if (is_status_ok(st))
      st = STATUS_NOT;
  return st;
}

bool db_variable::operator==(const db_variable &r) const {
  bool same = strcmp(r.fname, fname) == 0;
  same &= r.type == type;
  same &= r.len == len;
  if (same) {
    // flags
    same = r.flags.is_unique == flags.is_unique;
    same &= r.flags.can_be_null == flags.can_be_null;
  }
  return same;
}

bool db_variable::operator!=(const db_variable &r) const {
  return !(*this == r);
}

/* db_reference */
db_reference::db_reference(const std::string &fname, db_table ftable,
    const std::string &f_fname, bool is_fkey, db_reference_act on_del,
    db_reference_act on_upd)
  : fname(fname), foreign_table(ftable), foreign_fname(f_fname),
    is_foreign_key(is_fkey), has_on_delete(false), delete_method(on_del),
    update_method(on_upd) {
  if (delete_method != db_reference_act::ref_act_not)
    has_on_delete = true;
  if (update_method != db_reference_act::ref_act_not)
    has_on_update = true;
}

merror_t db_reference::CheckYourself() const {
  merror_t error = ERROR_SUCCESS_T;
  if (!fname.empty()) {
    if (!foreign_fname.empty()) {
      // перестраховались
      if (has_on_delete && delete_method == db_reference_act::ref_act_not) {
        error = ERROR_DB_VARIABLE;
        Logging::Append(io_loglvl::err_logs, "несоответсвие метода удаления "
            "для ссылки. Поле: " + fname + ". Внешнее поле: " + foreign_fname);
      }
      if (has_on_update && update_method == db_reference_act::ref_act_not) {
        error = ERROR_DB_VARIABLE;
        Logging::Append(io_loglvl::err_logs, "несоответсвие метода обновления "
            "для ссылки. Поле: " + fname + ". Внешнее поле: " + foreign_fname);
      }
    } else {
      error = ERROR_DB_VARIABLE;
      Logging::Append(io_loglvl::err_logs, "пустое имя поля для "
          "внешней таблицы бд\n");
    }
  } else {
    error = ERROR_DB_VARIABLE;
      Logging::Append(io_loglvl::err_logs, "пустое имя поля таблицы бд\n");
  }
  return error;
}

/* this is for postgresql, what about anothers? */
std::string db_reference::GetReferenceActString(db_reference_act act) {
  switch (act) {
    case db_reference_act::ref_act_not:
      return "";
    case db_reference_act::ref_act_set_null:
      return "SET NULL";
    case db_reference_act::ref_act_cascade:
      return "CASCADE";
    case db_reference_act::ref_act_restrict:
      return "RESTRICT";
    default:
      throw std::exception();
  }
}

bool db_reference::operator==(const db_reference &r) const {
  bool same = fname == r.fname;
  same |= foreign_table == r.foreign_table;
  same |= foreign_fname == r.foreign_fname;
  if (same) {
    same = is_foreign_key == r.is_foreign_key;
    same |= has_on_delete == r.has_on_delete;
    same |= has_on_update == r.has_on_update;
    same |= delete_method == r.delete_method;
    same |= update_method == r.update_method;
  }
  return same;
}

bool db_reference::operator!=(const db_reference &r) const {
  return !(*this == r);
}
}  // namespace asp_db
