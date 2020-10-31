/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_tables *
 *   Апишечка на взаимодействие функционала модуля базы данных
 * с представлением конкретных таблиц
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_TABLES_H_
#define _DATABASE__DB_TABLES_H_

#include "db_defines.h"
#include "db_queries_setup.h"
#include "db_queries_setup_select.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <stdint.h>

/* macro */
#define TABLE_FIELD_NAME(x) x##_NAME
#define TABLE_FIELD_PAIR(x) x, x##_NAME

/**
 * \brief Макро на добавление параметра к данным добавления в БД
 * \param field_flag Флаг наличия параметра - добавляем параметр
 *   если select_data.initialized & field_flag != 0x0
 * \param field_id Уникальный идентификатор поля в таблице
 * \param str Строковое представление значения поля
 *
 * \note Здесь пара упоминаний неинициализированных полей:
 *   src[db_query_insert_setup] Указатель структуру добавления данных к БД
 *   select_data[T шаблон] Ссылка на структуру/таблицу БД
 *   value[db_query_basesetup::row_values] Контейнер собранных значений
 *   i[db_query_basesetup::field_index] Индекс поля в контейнере полей таблицы
 * */
#define insert_macro(field_flag, field_id, str)  \
  {                                              \
    if (select_data.initialized & field_flag)    \
      if ((i = src->IndexByFieldId(field_id)) != \
          db_query_basesetup::field_index_end)   \
        values.emplace(i, str);                  \
  }

namespace asp_db {
namespace wns = where_nodes_setup;
#define UNDEFINED_TABLE 0x00000000
#define UNDEFINED_COLUMN 0x00000000

/**
 * \brief Класс исключений полученных при работе с объектами IDBTables
 * */
class idbtables_exception : public std::exception {
 public:
  idbtables_exception(const std::string& msg);
  idbtables_exception(class IDBTables* tables, const std::string& msg);
  idbtables_exception(class IDBTables* tables,
                      db_table t,
                      const std::string& msg);
  idbtables_exception(class IDBTables* tables,
                      db_table t,
                      db_variable_id id,
                      const std::string& msg);

  const char* what() const noexcept override;
  /**
   * \brief Добавить дополнительную, инициализированную информацию
   *   о таблице, о колонке таблицы(и т.д.) к сообщению исключения
   * */
  std::string WhatWithDataInfo() const;

  db_table GetTable() const;
  db_variable_id GetFieldID() const;

 protected:
  /// пространство таблиц
  IDBTables* tables_ = nullptr;
  /// идентификатор таблицы
  const db_table table_;
  /// идентификатор колонки таблицы
  db_variable_id field_id_;
  /// сообщение исключения
  std::string msg_;
};

/**
 * \brief Интерфейс объекта-связи таблиц для модуля базы данных
 * */
class IDBTables {
 public:
  virtual ~IDBTables() {}

  /**
   * \brief Получить имя пространства имён таблиц
   *
   * \return Название пространства имён таблиц
   * */
  virtual std::string GetTablesNamespace() const { return "IDBTables"; }
  /**
   * \brief Получить имя таблицы по её id
   * \param t Идентификатор таблицы
   *
   * \return Имя таблицы
   * */
  virtual std::string GetTableName(db_table t) const = 0;
  /**
   * \brief Получить id таблицы по её имени
   * \param tname Имя таблицы
   *
   * \return id таблицы
   * */
  virtual db_table StrToTableCode(const std::string& tname) const = 0;
  /**
   * \brief Получить указатель на список(контейнер) полей таблицы по её id
   * \param t Идентификатор таблицы
   *
   * \return Указатель на контейнер полей таблицы
   * */
  virtual const db_fields_collection* GetFieldsCollection(db_table t) const = 0;
  /**
   * \brief Получить имя поля первичного ключа, id иначе говоря
   * \param dt Идентификатор таблицы
   *
   * \return Строка с именем поля первичного ключа таблицы dt
   * */
  virtual std::string GetIdColumnName(db_table dt) const = 0;
  /**
   * \brief Получить ссылку сетап создания/обновления таблицы
   * \param dt Идентификатор таблицы
   *
   * \return Ссылка на сетап создания таблицы dt
   * */
  virtual const db_table_create_setup& CreateSetupByCode(db_table dt) const = 0;

  /**
   * \brief Шаблон функции получения имени таблицы по типу.
   *   Шаблон специализируется классами используемых
   *   таблиц в основной программе
   * \tparam Класс с++ структуры/имплементирующей таблицу БД
   *
   * \return Имя таблицы
   * */
  template <class TableI>
  std::string GetTableName() const {
    return "";
  }
  /**
   * \brief Шаблон функции получения кода таблицы по типу.
   *   Шаблон специализируется классами используемых
   *   таблиц в основной программе
   * \tparam Тип с++ структуры/имплементирующей таблицу БД
   *
   * \return id таблицы
   * */
  template <class TableI>
  db_table GetTableCode() const {
    return UNDEFINED_TABLE;
  }
  /**
   * \brief Получить сслылку на структуру данных поля таблицы по его id
   * \tparam table Класс с++-структуры/таблицы данных
   * \param id Идентификатор таблицы
   *
   * \return Ссылка на данные поля таблицы
   *
   * \throw idbtables_exception Если соответствующего поля не было обнаружено
   * */
  template <db_table table>
  const db_variable& GetFieldById(db_variable_id id) {
    const db_fields_collection* fc = GetFieldsCollection(table);
    if (fc) {
      for (auto f = fc->begin(); f != fc->end(); ++f)
        if (f->fid == id)
          return *f;
    } else {
      throw idbtables_exception(
          this, table, "IDBTables метод GetFieldCollection вернул nullptr");
    }
    throw idbtables_exception(
        this, table, id,
        "IDBTables не найдено поле с id " + std::to_string(id));
  }

  /**
   * \brief Собрать структуру/обёртку над входными
   *   данными для INSERT операции БД
   * \tparam Класс с++ структуры/имплементирующей таблицу БД
   * \param insert_data Ссылка на вектор(?контейнер)
   *   добавляемых в бд структур
   *
   * \return shared_ptr на структуру db_query_insert_setup
   * */
  template <class TableI>
  std::unique_ptr<db_query_insert_setup> InitInsertSetup(
      const std::vector<TableI>& insert_data) const {
    if (db_query_insert_setup::haveConflict(insert_data))
      return nullptr;
    db_table table = GetTableCode<TableI>();
    std::unique_ptr<db_query_insert_setup> ins_setup(
        new db_query_insert_setup(table, *GetFieldsCollection(table)));
    if (ins_setup.get() != nullptr)
      for (auto& x : insert_data)
        setInsertValues<TableI>(ins_setup.get(), x);
    return ins_setup;
  }

  /**
   * \brief Заполнить контейнер out_vec данными select структуры src
   * \param src Указатель на структуру результатов SELECT команды
   * \param out_vec Указатель на вектор выходных результатов
   * */
  template <class TableI>
  void SetSelectData(db_query_select_result* src,
                     std::vector<TableI>* out_vec) const;

  /**
   * \brief Собрать дерево условий по данным таблицы БД
   * \param where Ссылка на таблицу
   *
   * \note В такой конфигурации работает только для операции EQ.
   *   А вообще, даже подход не правилен - условия задаются вне
   *   контекста определённой структуры, а при работе с наборами
   *   данных. Доопределить, доделать, см todo секцию
   *
   * \return Указатель на собранное дерево условий
   *
   * \todo Переориентировать на работу не со структурой таблицы,
   *   а с сетапом, похожим на сетап создания и т.п.
   *   Например, через db_condition_node
   *
   * \note 2020.10.12 update
   *   Интерфейс остался, но логика подправилась - теперь такое
   *   неказистое деревцо запроса добавления и привязано к
   *   insert операции
   * */
  template <class TableI>
  std::shared_ptr<DBWhereClause<where_node_data>> InitInsertTree(
      const TableI& where) const {
    auto is = InitInsertSetup<TableI>({where});
    return (is.get() != nullptr) ? is->InitInsertTree() : nullptr;
  }
  /* nodes */
  template <db_table t, class Tval>
  wns::node_ptr Eq(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_eq);
  }
  template <db_table t, class Tval>
  wns::node_ptr Is(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_is);
  }
  template <db_table t, class Tval>
  wns::node_ptr Ne(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_ne);
  }
  template <db_table t, class Tval>
  wns::node_ptr Ge(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_ge);
  }
  template <db_table t, class Tval>
  wns::node_ptr Gt(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_gt);
  }
  template <db_table t, class Tval>
  wns::node_ptr Le(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_le);
  }
  template <db_table t, class Tval>
  wns::node_ptr Lt(db_variable_id field_id, const Tval& val) {
    return node_bind<t>(field_id, val, wns::node_lt);
  }
  /**
   * \brief Шаблон функции собирающей узлы `Like` операций для where
   *   условий.
   *
   * \tparam t Тип таблицы
   * \tparam Tval Тип данных
   *
   * \param field_id Идентификатор обновляемого поля
   * \param val
   *
   * \note Функция доступна только символьных полей и значений
   * */
  template <db_table t, class Tval>
  wns::node_ptr Like(db_variable_id field_id,
                     const Tval& val,
                     bool inverse = false) {
    wns::node_ptr like = nullptr;
    try {
      const db_variable& field = GetFieldById<t>(field_id);
      like =
          wns::node_like(field, field2str::translate(val, field.type), inverse);
    } catch (idbtables_exception& e) {
      // добавить к сообщению об ошибке дополнителоьную информацию
      Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
    }
    return like;
  }
  /**
   * \brief Шаблон функции собирающей узлы `Between` операций для where
   *   условий.
   *
   * \tparam t Тип таблицы
   * \tparam Tval Тип данных
   *
   * \param field_id Идентификатор обновляемого поля
   * \param min Нижняя граница значения параметров
   * \param max Верхняя граница изменения параметра
   * \param inverse Искать вне границ заданого участка
   * */
  template <db_table t, class Tval>
  wns::node_ptr Between(db_variable_id field_id,
                        const Tval& min,
                        const Tval& max,
                        bool inverse = false) {
    wns::node_ptr between = nullptr;
    try {
      const db_variable& field = GetFieldById<t>(field_id);
      between =
          wns::node_between(field, field2str::translate(min, field.type),
                            field2str::translate(max, field.type), inverse);
    } catch (idbtables_exception& e) {
      // добавить к сообщению об ошибке дополнителоьную информацию
      Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
    }
    return between;
  }

 protected:
  /**
   * \brief Шаблонная функция сбора сетапа добавления по структуре таблицы БД
   * \param src Указатель на сетап данных добавления к БД
   * \param select_data Ссылка на структуру таблицы
   *   для сбора сетапа добавления
   *
   * \note Шаблон закрыт, разрешены только спецификации ниже
   * */
  template <class T>
  void setInsertValues(db_query_insert_setup* src, const T& select_data) const;
  /**
   * \brief Инициализировать сетап добавления в БД
   * \param t Идентификатор таблицы
   * \param src Указатель на сетап добавления
   * \param insert_data Ссылка на добавляемые структуры таблиц БД
   *
   * \return Указатель на структуру добавления
   * */
  template <class DataInfo, class Table>
  std::shared_ptr<db_query_insert_setup> init(
      Table t,
      db_query_insert_setup* src,
      const std::vector<DataInfo>& insert_data) {
    if (haveConflict(insert_data))
      return nullptr;
    std::shared_ptr<db_query_insert_setup> ins_setup(
        new db_query_insert_setup(t, *GetFieldsCollection(t)));
    if (ins_setup.get() != nullptr)
      for (const auto& x : insert_data)
        setInsertValues<Table>(src, x);
    return ins_setup;
  }
  /**
   * \brief Разбить строковое представление массива данных
   * \param str Ссылка на строковое представление
   * \param cont Указатель на контейнер выходных данных
   * \param str2type Функциональный объект конвертации строкового
   *   представления значения к значению оригинального типа.
   *   На вход принимает ссылку на строку, выдаёт обект требуемого
   *   типа.
   *
   * \note Сделал ориентированно на Postgre, не знаю унифицирован
   *   ли формат возврата массива в SQL
   *
   * \todo Проблема с начальными и конечными скобками, добавил issue.
   *   UPD: Заменил '[' и ']' на '{' и '}'
   *
   * \return Код(статус) результата выполнения
   * */
  template <class Container>
  mstatus_t string2Container(
      const std::string& str,
      Container* cont,
      std::function<typename Container::value_type(const std::string&)>
          str2type = [](const std::string& s) { return s; }) const {
    std::string estr = str;
    estr.erase(std::remove(estr.begin(), estr.end(), '{'), estr.end());
    estr.erase(std::remove(estr.begin(), estr.end(), '}'), estr.end());
    mstatus_t st = STATUS_DEFAULT;
    if (!std::is_same<std::string, typename Container::value_type>::value) {
      // тип значений контейнера не строковый, нужна конвертация
      std::vector<std::string> tmp;
      split_str(estr, &tmp, ',');
      std::for_each(tmp.begin(), tmp.end(), [&cont, str2type](auto& s) {
        cont->push_back(str2type(s));
      });
    } else {
      // если нужно инициализировать контейнер строк, внесём изменения
      //   в этом же контейнере
      split_str(estr, cont, ',');
      // если даже строковое значение тоже необходимо переработать
      std::transform(cont->begin(), cont->end(), cont->begin(), str2type);
    }
    return st;
  }
  /**
   * \brief Функция создания двухпараметрических узлов - кроме `like`,
   *   `between`, `in`, без инвертирования
   * */
  template <db_table t, class Tval>
  wns::node_ptr node_bind(
      db_variable_id field_id,
      const Tval& val,
      std::function<wns::node_ptr(const db_variable&, const std::string&)>
          node_create) {
    wns::node_ptr node = nullptr;
    try {
      const db_variable& field = GetFieldById<t>(field_id);
      node = node_create(field, field2str::translate(val, field.type));
    } catch (idbtables_exception& e) {
      // добавить к сообщению об ошибке дополнителоьную информацию
      Logging::Append(io_loglvl::err_logs, e.WhatWithDataInfo());
    }
    return node;
  }
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_TABLES_H_
