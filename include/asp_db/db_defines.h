/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_defines *
 *   Здесь прописаны структуры-примитивы для обеспечения
 * взаимодействия с базой данных
 * ===================================================================
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_DEFINES_H_
#define _DATABASE__DB_DEFINES_H_

#include "asp_db/db_append_functor.h"

#include "asp_utils/Common.h"
#include "asp_utils/ErrorWrap.h"

#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <assert.h>
#include <stdint.h>

// todo: remove codes to ErrorWrap.h
/** \brief Ошибка работы с базой данных */
#define ERROR_DATABASE_T 0x0007

/** \brief Ошибка подключения к базе данных */
#define ERROR_DB_CONNECTION (0x0100 | ERROR_DATABASE_T)
#define ERROR_DB_CONNECTION_MSG "database connection error "
/** \brief Ошибка переменной БД */
#define ERROR_DB_VARIABLE (0x0200 | ERROR_DATABASE_T)
#define ERROR_DB_VARIABLE_MSG "database variable error "
/** \brief Ошибка поля ссылки */
#define ERROR_DB_REFER_FIELD (0x0300 | ERROR_DATABASE_T)
#define ERROR_DB_REFER_FIELD_MSG "database reference to another table error "
/** \brief Ошибка во время выполнения операции "существование таблицы" */
#define ERROR_DB_TABLE_EXISTS (0x0400 | ERROR_DATABASE_T)
#define ERROR_DB_TABLE_EXISTS_MSG "database table exist error "
/** \brief Ошибка составления запроса */
#define ERROR_DB_QUERY_SETUP (0x0500 | ERROR_DATABASE_T)
#define ERROR_DB_QUERY_SETUP_MSG "database query setup error "
/** \brief Ошибка первичного ключа */
#define ERROR_DB_TABLE_PKEY (0x0600 | ERROR_DATABASE_T)
#define ERROR_DB_TABLE_PKEY_MSG "database table primary key setup error "
/** \brief Ошибка SQL запроса */
#define ERROR_DB_SQL_QUERY (0x0700 | ERROR_DATABASE_T)
#define ERROR_DB_SQL_QUERY_MSG "database sql query exception "
/** \brief Ошибочная операция СУБД */
#define ERROR_DB_OPERATION (0x0800 | ERROR_DATABASE_T)
#define ERROR_DB_OPERATION_MSG "database wrong operation name "
/** \brief Колонка не существует */
#define ERROR_DB_COL_EXISTS (0x0900 | ERROR_DATABASE_T)
#define ERROR_DB_COL_EXISTS_MSG "database table column doesn't exists "
/** \brief Ошибка конфигурации точки сохранения */
#define ERROR_DB_SAVE_POINT (0x0A00 | ERROR_DATABASE_T)
#define ERROR_DB_SAVE_POINT_MSG "database save point error "

namespace asp_db {
using namespace asp_utils;
/**
 * \brief Макро на открытие приватных методов класса
 *   для другого класса.
 * \note Имплементация сильной связности
 *   классов "Включаемый"-"Включающий"
 * */
#define OWNER(x) friend class x
/**
 * \brief Идентификатор таблиц
 * */
typedef uint32_t db_table;
/**
 * \brief Уникальный идентификатор поля БД
 * */
typedef uint32_t db_variable_id;
/**
 * \brief Клиент БД
 * */
enum class db_client : uint32_t {
  NOONE = 0,
  /// реализация в db_connection_postgre.cpp
  POSTGRESQL = 1,
  /// реализация в db_connection_firebird.cpp
  FIREBIRD = 2
};
/**
 * \brief Получить имя клиента БД по идентификатору
 * */
std::string db_client_to_string(db_client client);

/**
 * \brief Структура описывающая столбец таблицы БД.
 *   собственно, описание валидно и для знчения в этом столбце,
 *   так что можно чекать формат/неформат
 * */
struct db_variable {
 public:
  /**
   * \brief Класс исключений db_variable
   * */
  class db_variable_exception : public std::exception {
   public:
    db_variable_exception(const std::string& msg);
    db_variable_exception(db_variable& v, const std::string& msg);

    const char* what() const noexcept override;

   private:
    /// Оригинальное сообщение исключения
    std::string orig_msg_;
    /// Информация о поле таблицы БД
    std::string var_info_;
    /// Результирующее сообщение
    std::string msg_;
  };

 public:
  /**
   * \brief Перечисление допустимых типов данных
   * \note Идея расширить функционал на вывод в файл.
   *   По сетап данным из db_queries_setup.h можно
   *   собирать столбцы данных, специализируя их тип и оотображение.
   *   Также можно расчитывать на реализацию обратного функционала -
   *   чтение из форматированных файлов.
   * \todo А как правильно разграничить 'text' и 'char_array'
   *   postgre вот не парится, text'ом назвал char_array
   * */
  enum class db_variable_type : uint32_t {
    /** пустой тип */
    type_empty = 0,
    /** автоинкрементируюмое поле id
     *   в postgresql это SERIAL */
    type_autoinc,
    /** для хранения Universal Unique Identificator(RFC 4122) */
    type_uuid,
    /** boolean */
    type_bool,
    /** 2байт int {-32,768; 32,767} */
    type_short,
    /** 4байт int {-2,147,483,648; 2,147,483,647} */
    type_int,
    /** 8байт int {-9,223,372,036,854,775,808; 9,223,372,036,854,775,807} */
    type_long,
    /** 4байт число с п.т. */
    type_real,
    /** дата */
    type_date,
    /** время */
    type_time,
    /** массив символов */
    type_char_array,
    /** просто текст(в postgresql такой есть) */
    type_text,
    /** поле сырых данных blob */
    type_blob
  };
  /**
   * \brief Флаги полей таблицы
   * */
  struct db_variable_flags {
   public:
    db_variable_flags() = default;
    db_variable_flags(const std::map<std::string, bool>& flags);

   public:
    /** \brief Значение явдяется первичным ключом */
    bool is_primary_key = false;
    /** \brief Значение является ссылкой на столбец другой таблицы */
    bool is_reference = false;
    /** \brief Значение может быть не инициализировано */
    bool can_be_null = true;
    /** \brief Число может быть отрицательным
     * \note only for numeric types */
    bool can_be_negative = false;
    /** \brief Значение представляет собой массив(актуально для типа char) */
    bool is_array = false;
    bool has_default = false;
  };

 public:
  /** \brief id столбца/параметра, чтоб создавать и апдейтить */
  db_variable_id fid;
  /**
   * \brief имя столбца/параметра, чтоб создавать и апдейтить
   * \todo да почему не стринга то?
   * */
  const char* fname;
  /** \brief тип значения */
  db_variable_type type;
  /** \brief флаги колонуи таблицы */
  db_variable_flags flags;
  /** \brief дефолтное значение, если есть */
  std::string default_str;
  /** \brief для массивов - количество элементов
   * \default 1 */
  int len;

 public:
  db_variable(db_variable_id fid,
              const char* fname,
              db_variable_type type,
              db_variable_flags flags,
              int len = 1);

  bool operator==(const db_variable& r) const;
  bool operator!=(const db_variable& r) const;

  /**
   * \brief Проверить несовместимы ли флаги и другие параметры
   * */
  merror_t CheckYourself() const;
  /**
   * \brief Конвертировать показания часов к строке времени в формате
   *   принятом asp_db
   * \param tm Указатель на данные часов
   *
   * \todo Рассмотреть возможность изменения формата часов, вероятно
   *   не здесь
   *
   * Формат времени в asp_db `hh:mm:ss`
   * */
  static std::string TimeToString(std::tm* tm);
  /**
   * \brief Конвертировать показания часов к строке даты в формате
   *   принятом в asp_db
   * \param tm Указатель на данные часов
   * \note см замечания к TimeToString
   *
   * Формат даты в asp_db `yyyy-mm-dd`
   * */
  static std::string DateToString(std::tm* tm);
  /**
   * \brief Сформировать из вектора значений одну строку.
   * \tparam CIterator Класс итератора по контейнеру
   * \tparam U Функтор конвертации значения к строке
   * \param to_str Унарная функция(функтор) преобразования
   *   значения итератора к строке
   * \note Так упаковывается массив параметров
   * */
  template <class CIteratorT, class U = pass<std::string>>
  static std::string TranslateFromVector(const CIteratorT& begin,
                                         const CIteratorT& end,
                                         U to_str = U()) {
    std::string result = "";
    for (auto it = begin; it != end; ++it) {
      auto str = to_str(*it);
      result += std::to_string(str.size()) + " " + str;
    }
    return result;
  }

  /**
   * \brief Разбить строку в вектор.
   * \note Так мы распаковываем вектор
   * */
  template <class A = AppendOp<std::vector<std::string>>>
  static mstatus_t TranslateToVector(const std::string& str, A op) {
    const char* ptr = str.c_str();
    size_t pos = 0;
    size_t start = 0;
    mstatus_t st = STATUS_OK;
    while (start < str.size()) {
      int n = std::atoi(ptr + start);
      pos = str.find(' ', start);
      if (pos != std::string::npos) {
        if (n > 0) {
          if (pos + n < str.size()) {
            op(str.substr(pos + 1, n));
            pos = pos + n;
          } else {
            st = STATUS_HAVE_ERROR;
            break;
          }
        } else {
          // ошибка формата строки
          throw db_variable_exception(
              std::string("Ошибка формата строки в функции TranslateToVector")
              + "\nВход функции: " + str);
        }
      } else {
        break;
      }
      start = pos + 1;
    }
    return st;
  }
};
using db_variable_type = db_variable::db_variable_type;
using db_variable_exception = db_variable::db_variable_exception;

/**
 * \brief Шаблон функтора приведения значений к строковому виду
 * */
struct field2str {
  // шаблон для чмсленных типов
  template <
      class T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
  std::string translate(const T& val, db_variable_type) {
    return std::to_string(val);
  }
  std::string translate(tm* val, db_variable_type type) {
    if (type == db_variable_type::type_date) {
      return db_variable::DateToString(val);
    } else if (type == db_variable_type::type_time) {
      return db_variable::TimeToString(val);
    }
    throw db_variable_exception(
        "Несоответствие приведения типов для времени/даты"
        "\nКод типа: "
        + std::to_string((uint32_t)type));
  }
  /**
   * \brief Перегрузка функции приведения данных контейнера к строковому
   *   представлению
   * */
  template <class T, class U = pass<std::string>>
  std::string translate(const std::vector<T>& c,
                        db_variable_type,
                        U to_str = U()) {
    return TranslateFromVector(c.begin(), c.end(), to_str);
  }
  std::string translate(const std::string& val, db_variable_type) {
    // todo: возможно здесь общий конвертер `строка`->`строку`, но
    //   учитывающий специфику обрабатываемого типа значения БД
    return val;
  }
  std::string translate(const char* val, db_variable_type) {
    return std::string(val);
  }
};

/* complex_pk */
/**
 * \brief структура содержащая параметры первичного ключа
 * */
struct db_complex_pk {
  std::vector<std::string> fnames;
};

/**
 * \brief enum действий над объектами со ссылками на другие элементами:
 *   колонками другой таблицы или самими таблицами
 * */
enum class db_reference_act {
  /** \brief Ничего не делать */
  ref_act_not = 0,
  /** \brief Установить в ноль */
  ref_act_set_null,
  /** \brief Установить значение по умолчанию
   * \note Оно посложнее в реализации, необходимо сначала будет
   *   подправить и оттестировать просто работу со значениями по умолчанию
   * \todo Собственно добавить */
  // ref_act_set_default,
  /** \brief Изменить зависимые объекты и объекты зависимые от зависимых */
  ref_act_cascade,
  /** \brief Не изменять объект при наличии связей */
  ref_act_restrict
};
// static_assert (0, "добавить нереализованные действия");

/* db_reference */
/**
 * \brief Структура содержащая параметры удалённого ключа
 * */
struct db_reference {
 public:
  /**
   * \brief Собственное имя параметра таблицы
   * */
  std::string fname;
  /**
   * \brief Таблица на которую ссылается параметр
   * \todo Заменить на обычную строку
   * */
  db_table foreign_table;
  /**
   * \brief имя параметра в таблице foreign_table
   * */
  std::string foreign_fname;

  /**
   * \brief флаг 'внешнего ключа'
   * */
  bool is_foreign_key = false;
  /**
   * \brief флаг наличия on_delete метода
   * */
  bool has_on_delete = false;
  /**
   * \brief флаг наличия on_update метода
   * */
  bool has_on_update = false;
  /**
   * \brief метод удаления значения
   * \default db_on_delete::empty
   * */
  db_reference_act delete_method;
  db_reference_act update_method;

 public:
  db_reference(const std::string& fname,
               db_table ftable,
               const std::string& f_fname,
               bool is_fkey,
               db_reference_act on_del = db_reference_act::ref_act_not,
               db_reference_act on_upd = db_reference_act::ref_act_not);

  bool operator==(const db_reference& r) const;
  bool operator!=(const db_reference& r) const;

  /** \brief Проверить несовместимы ли флаги и другие параметры */
  merror_t CheckYourself() const;

  /**
   * \brief Получить строковое представление update|delete action
   *
   * \throw std::exception (look todo)
   * */
  static std::string GetReferenceActString(db_reference_act act);
};

// todo: заменить на структуры с конструктором, принимающим
//   спискок инициализации
typedef std::vector<db_variable> db_fields_collection;
typedef std::vector<db_reference> db_ref_collection;
}  // namespace asp_db

#endif  // !_DATABASE__DB_DEFINES_H_
