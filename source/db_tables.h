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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <stdint.h>


/* macro */
#define TABLE_FIELD_NAME(x) x ## _NAME
#define TABLE_FIELD_PAIR(x) x, x ## _NAME

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
#define insert_macro(field_flag, field_id, str) \
  { if (select_data.initialized & field_flag) \
      if ((i = src->IndexByFieldId(field_id)) != \
          db_query_basesetup::field_index_end) \
        values.emplace(i, str); }

namespace asp_db {
#define UNDEFINED_TABLE         0x00000000
#define UNDEFINED_COLUMN        0x00000000

/**
 * \brief Интерфейс объекта-связи таблиц для модуля базы данных
 * */
class IDBTables {
public:
  virtual ~IDBTables() {}

  /**
   * \brief Получить имя таблицы по её id
   * \param t Идентификатор таблицы
   *
   * \return Имя таблицы
   * */
  virtual std::string GetTableName(db_table t) const = 0;
  /**
   * \brief Получить id таблицы по её имени
   *
   * \return id таблицы
   * */
  virtual db_table StrToTableCode(const std::string &tname) const = 0;
  /**
   * \brief Получить указатель на список(контейнер) полей таблицы по её id
   * \param t Идентификатор таблицы
   *
   * \return Указатель на контейнер полей таблицы
   * */
  virtual const db_fields_collection *GetFieldsCollection(db_table t) const = 0;
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
  virtual const db_table_create_setup &CreateSetupByCode(db_table dt) const = 0;

  /**
   * \brief Шаблон функции получения имени таблицы по типу.
   *   Шаблон специализируется классами используемых
   *   таблиц в основной программе
   *
   * \return Имя таблицы
   * */
  template <class T>
  std::string GetTableName() const { return ""; }
  /**
   * \brief Шаблон функции получения кода таблицы по типу.
   *   Шаблон специализируется классами используемых
   *   таблиц в основной программе
   *
   * \return id таблицы
   * */
  template <class T>
  db_table GetTableCode() const { return UNDEFINED_TABLE; }

  /**
   * \brief Собрать структуру/обёртку над входными
   *   данными для INSERT операции БД
   * \param insert_data Ссылка на вектор(?контейнер)
   *   добавляемых в бд структур
   *
   * \return shared_ptr на структуру db_query_insert_setup
   * */
  template <class TableI>
  std::unique_ptr<db_query_insert_setup> InitInsertSetup(
      const std::vector<TableI> &insert_data) const {
    if (db_query_insert_setup::haveConflict(insert_data))
      return nullptr;
    db_table table = GetTableCode<TableI>();
    std::unique_ptr<db_query_insert_setup> ins_setup(
        new db_query_insert_setup(table, *GetFieldsCollection(table)));
    if (ins_setup.get() != nullptr)
      for (auto &x: insert_data)
        setInsertValues<TableI>(ins_setup.get(), x);
    return ins_setup;
  }

  /**
   * \brief Заполнить контейнер out_vec данными select структуры src
   * \param src Указатель на структуру результатов SELECT команды
   * \param out_vec Указатель на вектор выходных результатов
   * */
  template <class TableI>
  void SetSelectData(db_query_select_result *src,
      std::vector<TableI> *out_vec) const;

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
   * */
  template <class TableI>
  std::unique_ptr<db_where_tree> InitWhereTree(const TableI &where) const {
    return db_where_tree::init(InitInsertSetup<TableI>({where}).get());
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
  void setInsertValues(db_query_insert_setup *src, const T &select_data) const;
  /**
   * \brief Инициализировать сетап добавления в БД
   * \param t Идентификатор таблицы
   * \param src Указатель на сетап добавления
   * \param insert_data Ссылка на добавляемые структуры таблиц БД
   *
   * \return Указатель на структуру добавления
   * */
  template <class DataInfo, class Table>
  std::shared_ptr<db_query_insert_setup> init(Table t,
      db_query_insert_setup *src, const std::vector<DataInfo> &insert_data) {
    if (haveConflict(insert_data))
      return nullptr;
    std::shared_ptr<db_query_insert_setup> ins_setup(
        new db_query_insert_setup(t, *GetFieldsCollection(t)));
    if (ins_setup.get() != nullptr)
      for (const auto &x: insert_data)
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
   template<class Container>
   mstatus_t string2Container(const std::string &str, Container *cont,
       std::function<typename Container::value_type(const std::string &)>
       str2type = [] (const std::string &s) { return s; }) const {
     std::string estr = str;
     estr.erase(std::remove(estr.begin(), estr.end(), '{'), estr.end());
     estr.erase(std::remove(estr.begin(), estr.end(), '}'), estr.end());
     mstatus_t st = STATUS_DEFAULT;
     if (!std::is_same<std::string, typename Container::value_type>::value) {
       // тип значений контейнера не строковый, нужна конвертация
       std::vector<std::string> tmp;
       split_str(estr, &tmp, ',');
       std::for_each(tmp.begin(), tmp.end(),
           [&cont, str2type](auto &s) { cont->push_back(str2type(s)); });
     } else {
       // если нужно инициализировать контейнер строк, внесём изменения
       //   в этом же контейнере
       split_str(estr, cont, ',');
       // если даже строковое значение тоже необходимо переработать
       std::transform(cont->begin(), cont->end(), cont->begin(), str2type);
     }
     return st;
   }
};
}  // namespace asp_db

#endif  // !_DATABASE__DB_TABLES_H_
