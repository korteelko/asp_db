/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * db_tables *
 *   Апишечка на взаимодействие функционала модуля базы данных
 * с представлением конкретных таблиц
 * ===================================================================
 *
 * Copyright (c) 2020-2021 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_TABLES_H_
#define _DATABASE__DB_TABLES_H_

#include "asp_db/db_defines.h"
#include "asp_db/db_queries_setup.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <vector>

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
#define insert_macro(field_flag, field_id, str)   \
  {                                               \
    if (select_data.initialized & field_flag)     \
      if ((i = src->IndexByFieldId(field_id))     \
          != db_query_basesetup::field_index_end) \
        values.emplace(i, str);                   \
  }

namespace asp_db {
#define UNDEFINED_TABLE 0x00000000
#define UNDEFINED_COLUMN 0x00000000

class IDBTables;
/**
 * \brief Класс исключений полученных при работе с объектами IDBTables
 * */
template <db_table table>
class idbtables_exception : public std::exception {
 public:
  explicit idbtables_exception(std::string&& msg)
      : tables_(nullptr), msg_(msg) {}
  idbtables_exception(class IDBTables* tables, std::string&& msg)
      : tables_(tables), msg_(msg) {}

  idbtables_exception(class IDBTables* tables,
                      db_variable_id id,
                      std::string&& msg)
      : tables_(tables), field_id_(id), msg_(msg) {}

  const char* what() const noexcept override { return msg_.c_str(); }
  /**
   * \brief Добавить дополнительную, инициализированную информацию
   *   о таблице, о колонке таблицы(и т.д.) к сообщению исключения
   * */
  std::string WhatWithDataInfo() const;
  db_table GetTable() const { return table; }
  db_variable_id GetFieldID() const { return field_id_; }

 protected:
  /// пространство таблиц
  IDBTables* tables_ = nullptr;
  /// идентификатор колонки таблицы
  db_variable_id field_id_;
  /// сообщение исключения
  std::string msg_;
};

/**
 * \brief Интерфейс объекта-связи таблиц для модуля базы данных
 *
 * \todo Переместить все методы относящиеся к сборке условий в отдельный класс,
 *   агрегирующий(ссылающийся) на интерфейс IDBTables
 * */
class IDBTables {
 public:
  virtual ~IDBTables() = default;

  /**
   * \brief Получить имя пространства имён таблиц
   *
   * \return Название пространства имён таблиц
   *
   * Удобно на обработке исключений, если например сразу несколько
   * интерфейсов IDBTables создано
   * */
  virtual std::string GetTablesNamespace() const = 0;
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
  const db_variable& GetFieldById(db_variable_id id);
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
      const std::vector<TableI>& insert_data) const;
  /**
   * \brief Заполнить контейнер out_vec данными select структуры src
   * \param src Указатель на структуру результатов SELECT команды
   * \param out_vec Указатель на вектор выходных результатов
   * */
  template <class TableI>
  void SetSelectData(struct db_query_select_result* src,
                     std::vector<TableI>* out_vec) const;
  /**
   * \brief Собрать дерево условий insert запроса по данным
   *   структуры/таблицы БД
   * \tparam Класс с++ структуры/имплементирующей таблицу БД
   * \param where Ссылка на таблицу
   *
   * \return Указатель на собранное дерево условий
   * */
  template <class TableI>
  std::shared_ptr<DBWhereClause<where_node_data>> InitInsertTree(
      const TableI& where) const {
    auto is = InitInsertSetup<TableI>({where});
    return (is.get() != nullptr) ? is->InitInsertTree() : nullptr;
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
  void setInsertValues(db_query_insert_setup* src, const T& select_data) const {
    assert(0 && "not implemented function");
  }
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
      const std::vector<DataInfo>& insert_data);
  /**
   * \brief Разбить строковое представление массива данных
   * \param str Ссылка на строковое представление
   * \param cont Указатель на контейнер выходных данных
   * \param str2type Функциональный объект конвертации строкового
   *   представления значения к значению оригинального типа.
   *   На вход принимает ссылку на строку, выдаёт обект требуемого
   *   типа.
   *
   * \return Код(статус) результата выполнения
   *
   * \note Сделал ориентированно на Postgre, не знаю унифицирован
   *   ли формат возврата массива в SQL
   * */
  template <class Container>
  mstatus_t string2Container(
      const std::string& str,
      Container* cont,
      std::function<typename Container::value_type(const std::string&)>
          str2type = [](const std::string& s) { return s; }) const;
};

/* templates */

template <db_table table>
std::string idbtables_exception<table>::WhatWithDataInfo() const {
  std::string msg = msg_;
  if (tables_) {
    msg = "Table: " + tables_->GetTableName(table) + "\nField id: "
          + std::to_string(field_id_) + "\nOriginal message: '" + msg_ + "'";
  }
  return msg;
}

template <db_table table>
const db_variable& IDBTables::GetFieldById(db_variable_id id) {
  const db_fields_collection* fc = GetFieldsCollection(table);
  if (fc) {
    const auto field = std::find_if(
        fc->begin(), fc->end(), [id](const auto& it) { return it.fid == id;
        });
    if (field != fc->end())
      return *field;
  } else {
    throw idbtables_exception<table>(
        this, "IDBTables метод GetFieldCollection вернул nullptr");
  }
  throw idbtables_exception<table>(
      this, id, "IDBTables не найдено поле с id " + std::to_string(id));
}

template <class TableI>
std::unique_ptr<db_query_insert_setup> IDBTables::InitInsertSetup(
    const std::vector<TableI>& insert_data) const {
  if (db_query_insert_setup::haveConflict(insert_data))
    return nullptr;
  db_table table = GetTableCode<TableI>();
  std::unique_ptr<db_query_insert_setup> ins_setup(
      new db_query_insert_setup(table, *GetFieldsCollection(table)));
  if (ins_setup)
    for (auto& x : insert_data)
      setInsertValues<TableI>(ins_setup.get(), x);
  return ins_setup;
}

template <class DataInfo, class Table>
std::shared_ptr<db_query_insert_setup> IDBTables::init(
    Table t,
    db_query_insert_setup* src,
    const std::vector<DataInfo>& insert_data) {
  if (haveConflict(insert_data))
    return nullptr;
  std::shared_ptr<db_query_insert_setup> ins_setup(
      new db_query_insert_setup(t, *GetFieldsCollection(t)));
  if (ins_setup)
    for (const auto& x : insert_data)
      setInsertValues<Table>(src, x);
  return ins_setup;
}

template <class Container>
mstatus_t IDBTables::string2Container(
    const std::string& str,
    Container* cont,
    std::function<typename Container::value_type(const std::string&)> str2type)
    const {
  std::string estr = str;
  estr.erase(std::remove(estr.begin(), estr.end(), '{'), estr.end());
  estr.erase(std::remove(estr.begin(), estr.end(), '}'), estr.end());
  mstatus_t st = STATUS_DEFAULT;
  if (!std::is_same<std::string, typename Container::value_type>::value) {
    // тип значений контейнера не строковый, нужна конвертация
    std::vector<std::string> tmp;
    split_str(estr, &tmp, ',');
    std::for_each(tmp.begin(), tmp.end(),
                  [&cont, str2type](auto& s) { cont->push_back(str2type(s)); });
  } else {
    // если нужно инициализировать контейнер строк, внесём изменения
    //   в этом же контейнере
    split_str(estr, cont, ',');
    // если даже строковое значение тоже необходимо переработать
    std::transform(cont->begin(), cont->end(), cont->begin(), str2type);
  }
  return st;
}
}  // namespace asp_db

#endif  // !_DATABASE__DB_TABLES_H_
