/**
 * asp_therm - implementation of real gas equations of state
 * ===================================================================
 * * FileURL *
 *   В файле описан функционал оборачивающий адрессацию данных
 * из внешних источников
 * ===================================================================
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef UTILS__FILEURL_H
#define UTILS__FILEURL_H

#include "Common.h"
#include "ErrorWrap.h"

#include <string>


namespace file_utils {
/* todo: create class file_utils exception */
class FileURLRoot;

/** \brief тип задания адреса файла, буффера памяти и т.п. */
enum class url_t {
  empty = 0,
  /** \brief путь в файловой системе */
  fs_path = 1
};

/* todo: replace 'root' type with struct wth overloaded operator[]
 *   for c++17 comp */
/** \brief сетап URL - хост, юзер и т.п., все входные
  *   данные для фабрики URL объектов короч
  * \note сейчас то это просто для красоты */
struct SetupURL {
public:
  SetupURL(url_t url_type, const std::string &root);
  /** \brief Полный префикс для пути(от начала до рута) */
  std::string GetFullPrefix();
  /** \brief Тип ссылки */
  url_t GetURLType();

public:
  /** \brief тип адресов для создания полных путей */
  url_t url_type;
  /** \brief путь к корню системы(для файловой системы) */
  std::string root;
};
inline std::string SetupURL::GetFullPrefix() { return root; }
inline url_t SetupURL::GetURLType() { return url_type; }

/** \brief класс пути(м.б. обычный пути в файловой системе, урл,
  *   запрос к дб и т.п.)
  * \note немножко ликбеза:
  *   вид File(URI):
  *     "file://<host>/<path>"
  *   А вот обощённый URL несёт много больше информации:
  *     "<схема>:[//[<логин>[:<пароль>]@]<хост>[:<порт>]][/<URL‐путь>][?<параметры>][#<якорь>]"
  *   Естественно, полный url нам пока не нужен, но, лучше ввести лишнюю
  *     структуру хранящую все параметры подключения(хост, юзер с поролем),
  *     которую будем хранить в фабрике */
class FileURL {
  friend class FileURLRoot;

public:
  /** \brief Полный путь к файлу, области памяти */
  std::string GetURL() const;
  /** \brief Получить код ошибки */
  merror_t GetError() const;

  /** \brief Путь отмечен как невалидный */
  bool IsInvalidPath() const;
  /** \brief Установить код ошибки 'error' */
  void SetError(merror_t error);
  /** \brief Установить код ошибки 'error' и сообщение ошибки 'msg' */
  void SetError(merror_t error, const std::string &msg);
  /** \brief Залогировать ошибки */
  void LogError();

private:
  FileURL();
  FileURL(url_t url_type, const std::string &path);

private:
  ErrorWrap error_;
  mstatus_t status_;
  /** \brief тип адреса */
  url_t url_type_;
  /** \brief полный путь к файлу */
  std::string absolute_path_;
};
inline std::string FileURL::GetURL() const { return absolute_path_; }
inline merror_t FileURL::GetError() const { return error_.GetErrorCode(); }


/** \brief Фабрика инициализации файловых адресов */
class FileURLRoot {
public:
  FileURLRoot(const SetupURL &setup);
  FileURLRoot(url_t url_type, const std::string &root);

  /** \brief Инициализация прошла успешно */
  bool IsInitialized();
  /** \brief Собрать адрес файла по относительному пути */
  FileURL CreateFileURL(const std::string &relative_path);

private:
  /** \brief Проверить root директорию файловой системы */
  void check_fs_root();
  /** \brief Собрать адрес файла для случая файловой системы */
  FileURL set_fs_path(const std::string &relative_path);
  /** \brief Проверить что путь относительный, а не абсолютный */
  bool is_absolute_path(const std::string &path);

private:
  mstatus_t status_;
  SetupURL setup_;
};
}  // namespace file_unils

#endif  // !UTILS__FILEURL_H
